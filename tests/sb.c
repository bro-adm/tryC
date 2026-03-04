#include <criterion/criterion.h>
#include <stddef.h>
#include "tryC/sb.h"
#include "tryC/hal.h"
#include <unistd.h>

#define SB_TEST_PATH "sb_block.bin"
#define BIG_DISK_SIZE (1024 * 1024) // 1MB

static disk_t *sb_disk = NULL;

void sb_suite_setup(void) {
    FILE* f = fopen(SB_TEST_PATH, "wb");
    if (f) {
        ftruncate(fileno(f), BIG_DISK_SIZE); 
        fclose(f);
    }
    raid_result_t res = init_disk(&sb_disk, SB_TEST_PATH, 0);

    cr_assert_eq(res, RAID_SUCCESS, "Setup failed: Could not initialize sb_disk");
}

void sb_suite_teardown(void) {
    if (sb_disk) close_disk(&sb_disk);
    unlink(SB_TEST_PATH);
}

TestSuite(sb_suite, .init = sb_suite_setup, .fini = sb_suite_teardown);

Test(sb_suite, initialization) {
    superblock_t sb = {0};
    raid_config_t config = { .level = 1, .num_disks = 2, .chunk_size = 64, .version = 1 };
    uint8_t uuid[16] = {0xAA, 0xBB, 0xCC};

    cr_assert_eq(init_sb_t(sb_disk, &config, uuid, &sb), RAID_SUCCESS);

    // Verify Endian-converted fields
    cr_expect_eq(sb.raid_signature, htole32(RAID_SIGNATURE));
    cr_expect_eq(sb.disk_capacity, htole64(sb_disk->num_lbs));
    
    // Verify checksum is generated
    cr_expect_neq(sb.checksum, 0, "Checksum was not calculated!");
}

Test(sb_suite, lifecycle_roundtrip) {
    superblock_t sb_w = {0}, sb_r = {0};
    raid_config_t config = { .level = 5, .num_disks = 4, .chunk_size = 128, .version = 2 };
    uint8_t uuid[16] = {0x12, 0x34};

    init_sb_t(sb_disk, &config, uuid, &sb_w);
    cr_assert_eq(write_sb(sb_disk, &sb_w), RAID_SUCCESS);
    cr_assert_eq(read_sb(sb_disk, &sb_r), RAID_SUCCESS);

    // Note: read_sb converts back to Host Endian, so we compare sb_r to the original config, NOT sb_w!
    cr_expect_eq(sb_r.raid_signature, RAID_SIGNATURE);
    cr_expect_eq(sb_r.raid_level, 5);
    cr_expect_eq(sb_r.num_disks, 4);
    cr_expect_arr_eq(sb_r.raid_uuid, uuid, 16);
}

Test(sb_suite, read_catches_corruption) {
    superblock_t sb_w = {0};
    raid_config_t config = { .level = 1, .num_disks = 2, .chunk_size = 64, .version = 1 };
    init_sb_t(sb_disk, &config, (uint8_t[16]){0xAA}, &sb_w);
    write_sb(sb_disk, &sb_w);

    uint8_t raw_block[RAID_BLOCK_SIZE];
    read_blocks(sb_disk, SUPERBLOCK_OFFSET, 1, raw_block);
    
    // Test 1: Corrupt a numerical field (raid_level)
    raw_block[offsetof(superblock_t, raid_level)] ^= 0xFF; 
    write_blocks(sb_disk, SUPERBLOCK_OFFSET, 1, raw_block);
    cr_expect_eq(read_sb(sb_disk, &(superblock_t){0}), RAID_ERR_CHECKSUM);

    // Test 2: Corrupt the UUID array (raw bytes)
    read_blocks(sb_disk, SUPERBLOCK_OFFSET, 1, raw_block); // Reset
    raw_block[offsetof(superblock_t, raid_uuid) + 5] ^= 0xAA; 
    write_blocks(sb_disk, SUPERBLOCK_OFFSET, 1, raw_block);
    cr_expect_eq(read_sb(sb_disk, &(superblock_t){0}), RAID_ERR_CHECKSUM);
}

Test(sb_suite, signature_verification) {
    superblock_t sb = {0};
    raid_config_t config = { .level = RAID_0, .num_disks = 3, .chunk_size = 8 };
    init_sb_t(sb_disk, &config, (uint8_t[16]){0}, &sb);

    // Sabotage the signature
    sb.raid_signature = 0xDEADC0DE;
    write_sb(sb_disk, &sb);

    superblock_t sb_read;
    cr_expect_eq(read_sb(sb_disk, &sb_read), RAID_ERR_SIGNATURE, "Failed to catch invalid signature");
}

Test(sb_suite, write_pads_with_zeros) {
    superblock_t sb = {0};
    raid_config_t config = { .level = 1, .num_disks = 2, .chunk_size = 64, .version = 1 };
    uint8_t uuid[16] = {0};
    init_sb_t(sb_disk, &config, uuid, &sb);

    cr_assert_eq(write_sb(sb_disk, &sb), RAID_SUCCESS);

    uint8_t raw_block[RAID_BLOCK_SIZE];
    cr_assert_eq(read_blocks(sb_disk, SUPERBLOCK_OFFSET, 1, raw_block), RAID_SUCCESS);

    // Verify bounds strictly
    for (size_t i = sizeof(superblock_t); i < RAID_BLOCK_SIZE; i++) {
        cr_expect_eq(raw_block[i], 0, "Padding leak at byte %zu!", i);
    }
}

Test(sb_suite, superblock_data_isolation) {
    superblock_t sb_w = {0};
    raid_config_t config = { .level = RAID_0, .num_disks = 4, .chunk_size = 8 };
    init_sb_t(sb_disk, &config, (uint8_t[16]){0xAA}, &sb_w);

    // 1. Write a known pattern to the "User Data" area (LBA 1)
    uint8_t user_pattern[RAID_BLOCK_SIZE];
    memset(user_pattern, 0x55, RAID_BLOCK_SIZE);
    cr_assert_eq(write_blocks(sb_disk, 1, 1, user_pattern), RAID_SUCCESS);

    // 2. Write the Superblock to LBA 0
    cr_assert_eq(write_sb(sb_disk, &sb_w), RAID_SUCCESS);

    // 3. READ BACK LBA 1: Did the Superblock overwrite it?
    uint8_t user_check[RAID_BLOCK_SIZE];
    cr_assert_eq(read_blocks(sb_disk, 1, 1, user_check), RAID_SUCCESS);

    // If your write_sb logic is wrong (e.g. it writes 1024 bytes instead of 512),
    // this check will fail.
    cr_expect_arr_eq(user_pattern, user_check, RAID_BLOCK_SIZE, "CRITICAL: write_sb leaked into LBA 1!");

    // 4. READ BACK SUPERBLOCK: Did it stay clean?
    superblock_t sb_r;
    cr_assert_eq(read_sb(sb_disk, &sb_r), RAID_SUCCESS);
    cr_expect_eq(sb_r.raid_signature, RAID_SIGNATURE);
}
