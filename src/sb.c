#include "tryC/sb.h"
#include "tryC/defs.h"
#include "tryC/hal.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>


static uint32_t calculate_sb_checksum(const superblock_t *sb) {
    assert(sb != NULL);

    const uint8_t *data = (const uint8_t *)sb;
    
    // The checksum field itself and the padding after it are ignored.
    size_t len = offsetof(superblock_t, checksum);
    
    uint32_t hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return hash;
}

raid_result_t init_sb_t(const disk_t *disk, const raid_config_t* config, const uint8_t uuid[16], superblock_t *sb) {
    assert(disk != NULL);
    assert(config != NULL);
    assert(uuid != NULL);
    assert(sb != NULL);

    // zero init - safety fisrt
    memset(sb, 0, sizeof(superblock_t)); 

    // force standard Little Endian disk format
    sb->raid_signature = htole32(RAID_SIGNATURE);
    sb->raid_version   = htole32((uint32_t)config->version);
    sb->raid_level     = htole32((uint32_t)config->level);
    
    // Arrays of bytes are inherently Endian-safe
    memcpy(sb->raid_uuid, uuid, 16);

    sb->disk_id        = htole32(disk->id);
    sb->num_disks      = htole32(config->num_disks);
    sb->chunk_size     = htole32(config->chunk_size);
    
    sb->disk_capacity  = htole64(disk->num_lbs);

    sb->checksum = htole32(calculate_sb_checksum(sb)); 

    return RAID_SUCCESS;
}

raid_result_t write_sb(disk_t *disk, const superblock_t *sb){
    assert(disk != NULL);
    assert(sb != NULL);

    uint8_t sb_lb[RAID_BLOCK_SIZE] = {0};
    memcpy(sb_lb, sb, sizeof(superblock_t));

    return write_blocks(disk, SUPERBLOCK_OFFSET, SUPERBLOCK_SIZE, sb_lb);
}

raid_result_t read_sb(disk_t *disk, superblock_t *sb) {
    assert(disk != NULL);
    assert(sb != NULL);
    
    uint8_t sb_lb[RAID_BLOCK_SIZE] = {0};
    
    raid_result_t res = read_blocks(disk, SUPERBLOCK_OFFSET, SUPERBLOCK_SIZE, sb_lb);
    if (res != RAID_SUCCESS) return res;
    
    memcpy(sb, sb_lb, sizeof(superblock_t));

    /*
     * little endian -> convert to host
     * arrays dont need convert
     * padding dont need convert
     *
     * validate - checksum and signature
    */


    sb->raid_signature = le32toh(sb->raid_signature);
    if (sb->raid_signature != RAID_SIGNATURE) return RAID_ERR_SIGNATURE;
    
    uint32_t stored_checksum = le32toh(sb->checksum);
    if (stored_checksum != calculate_sb_checksum(sb)) return RAID_ERR_CHECKSUM;

    sb->raid_version   = le32toh(sb->raid_version);
    sb->raid_level     = le32toh(sb->raid_level);
    sb->disk_id        = le32toh(sb->disk_id);
    sb->num_disks      = le32toh(sb->num_disks);
    sb->chunk_size     = le32toh(sb->chunk_size);
    sb->disk_capacity  = le64toh(sb->disk_capacity);

    return RAID_SUCCESS;
}

// fully ai generated - i dont have the power for print logic :(
void print_sb(const superblock_t* sb) {
    assert(sb != NULL);

    printf("--- RAID Superblock Metadata ---\n");
    printf("Signature:  0x%08X (%s)\n", sb->raid_signature, 
           (sb->raid_signature == RAID_SIGNATURE ? "VALID" : "INVALID"));
    printf("Version:    %u\n", sb->raid_version);
    
    // Print UUID in standard 8-4-4-4-12 hex format
    printf("UUID:       ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sb->raid_uuid[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) printf("-");
    }
    printf("\n");

    printf("RAID Level: %u\n", sb->raid_level);
    printf("Disk ID:    %u (of %u total disks)\n", sb->disk_id, sb->num_disks);
    printf("Chunk Size: %u blocks\n", sb->chunk_size);
    
    // Safe, cross-platform 64-bit printing
    printf("Capacity:   %" PRIu64 " blocks\n", sb->disk_capacity);
    
    // Ensure checksum prints in native Host format
    printf("Checksum:   0x%08X\n", le32toh(sb->checksum));
    printf("--------------------------------\n");
}

