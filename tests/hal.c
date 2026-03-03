#include <criterion/criterion.h>
#include "tryC/hal.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_DISK_PATH "hal_fixture.bin"
#define TEST_DISK_SIZE (1024 * 1024) 

static disk_t *active_disk = NULL;

void hal_suite_setup(void) {
    FILE* f = fopen(TEST_DISK_PATH, "wb");
    if (f) {
        ftruncate(fileno(f), TEST_DISK_SIZE);
        fclose(f);
    }

    raid_result_t res = init_disk(&active_disk, TEST_DISK_PATH, 0);
    cr_assert_eq(res, RAID_SUCCESS, "HAL Setup: Failed to initialize disk fixture");
}

void hal_suite_teardown(void) {
    if (active_disk) {
        close_disk(&active_disk); // close disk is safe and doent double close...
    }
    unlink(TEST_DISK_PATH);
}

TestSuite(hal_suite, .init = hal_suite_setup, .fini = hal_suite_teardown);

Test(hal_suite, val_init) {
    cr_expect_eq(active_disk->num_lbs, 2048, "Disk should have exactly 2048 blocks");
    cr_expect_eq(active_disk->id, 0);
}

Test(hal_suite, write_read) {
    uint8_t w_buf[RAID_BLOCK_SIZE] = {0xDE, 0xAD, 0xBE, 0xEF}; // one byte per slot -> all the others are zero initialized
    uint8_t r_buf[RAID_BLOCK_SIZE] = {0};

    cr_assert_eq(write_blocks(active_disk, 100, 1, w_buf), RAID_SUCCESS);
    cr_assert_eq(read_blocks(active_disk, 100, 1, r_buf), RAID_SUCCESS);

    cr_assert_arr_eq(w_buf, r_buf, RAID_BLOCK_SIZE);
}

Test(hal_suite, multi_block_io) {
    size_t num_blocks = 4;
    size_t total_size = num_blocks * RAID_BLOCK_SIZE;

    uint8_t big_w_buf[total_size];
    uint8_t big_r_buf[total_size];
    
    for(size_t i = 0; i < total_size; i++) {
        big_w_buf[i] = (uint8_t)(i % 256);
    }

    // Offset 2044 is the LAST valid 4-block window (2044, 2045, 2046, 2047)
    // Offset 10 is very safe.
    cr_assert_eq(write_blocks(active_disk, 10, num_blocks, big_w_buf), RAID_SUCCESS);
    cr_assert_eq(read_blocks(active_disk, 10, num_blocks, big_r_buf), RAID_SUCCESS);

    cr_assert_arr_eq(big_w_buf, big_r_buf, total_size);
}

Test(hal_suite, lb_mappings) {
    raid_config_t config = { .level = RAID_0, .num_disks = 2, .chunk_size = 4 };
    disk_offset_t out;

    // LBA 5: 2 disks, 4-block chunks. 
    // LBA 0-3 -> Disk 0. LBA 4-7 -> Disk 1.
    get_disk_offset(5, &config, &out);
    
    cr_expect_eq(out.id, 1, "LBA 5 should be on Disk 1");
    cr_expect_eq(out.offset, 1, "LBA 5 should be at Offset 1 of the 1st chunk on Disk 1");
}

Test(hal_suite, availability_active) {
    cr_expect_eq(active_disk->state, DISK_ACTIVE, "Expected initial disk state to be active for this test to run"); // pretest expectation

    // 1. Probing a healthy disk should succeed and keep it ACTIVE
    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS);
    cr_assert_eq(active_disk->state, DISK_ACTIVE);
}

Test(hal_suite, availability_missing) {
    int original_fd = active_disk->fd;
    active_disk->fd = -1;

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS); // Function returns success because it handled the check
    cr_assert_eq(active_disk->state, DISK_MISSING, "Disk with fd -1 should be marked MISSING");

    active_disk->fd = original_fd;
}

Test(hal_suite, availability_io_failure) {
    int real_fd = active_disk->fd;
    close(real_fd); 

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_ERR_IO, "Probe should fail if FD is closed externally");
    cr_assert_eq(active_disk->state, DISK_FAILED, "Disk should move to FAILED state on probe failure");

    active_disk->fd = -1; 
}

Test(hal_suite, availability_rebuild_preservation) {
    active_disk->state = DISK_REBUILDING;

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS);
    cr_assert_eq(active_disk->state, DISK_REBUILDING, "Check should not overwrite REBUILDING state");
}

Test(hal_suite, out_of_bounds_assert_crash, .signal = SIGABRT) {
    uint8_t buf[RAID_BLOCK_SIZE] = {0};
    // Attempting to read block 5000 on a 2048-block disk
    read_blocks(active_disk, 5000, 1, buf); 
}

