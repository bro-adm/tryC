#include "tryC/hal.h"
#include "tryC/defs.h"

#include <assert.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

raid_result_t init_disk(disk_t **disk, const char *path, uint32_t id){
    assert(disk != NULL);
    if (path == NULL) return RAID_ERR_INVALID_VAL;

    disk_t* disk_p = (disk_t *)calloc(1, sizeof(disk_t));
    if (disk_p == NULL) return RAID_ERR_NOMEM;

    disk_p -> path = strdup(path);
    if (disk_p -> path == NULL){
        free(disk_p);
        return RAID_ERR_NOMEM;
    }

    // -1 represent error (in general negative is error)
    disk_p -> fd = open(path, O_RDWR);
    if (disk_p -> fd < 0 ) {
        perror("RAID HAL: Failed to open disk path");
        free(disk_p -> path);
        free(disk_p);
        return RAID_ERR_IO;
    }

    // -1 of type off_t is error
    off_t byte_size = lseek(disk_p -> fd, 0, SEEK_END);
    if (byte_size == (off_t)-1) {
        close(disk_p->fd);
        free(disk_p->path);
        free(disk_p);
        return RAID_ERR_IO;
    }
    lseek(disk_p -> fd, 0, SEEK_SET); // reset offset to start of disk
    disk_p->num_pbs = (uint64_t)byte_size / RAID_BLOCK_SIZE;

    disk_p -> id = id;
    disk_p -> state = DISK_ACTIVE;

    *disk = disk_p;
    return RAID_SUCCESS;
}

raid_result_t check_disk_availability(disk_t* disk) {
    assert(disk != NULL);

    // Already Missing or Closed
    if (disk->fd < 0) {
        disk->state = DISK_MISSING;
        return RAID_SUCCESS; 
    }

    // Disk Probe
    char buf;
    ssize_t probe = pread(disk->fd, &buf, 0, 0);

    if (probe < 0) {
        perror("RAID HAL: Disk probe failed");
        disk->state = DISK_FAILED;
        return RAID_ERR_IO;
    }

    // Disk Rebuild Managed by Superblock
    if (disk->state == DISK_REBUILDING) {
        return RAID_SUCCESS; 
    }

    // Active
    disk->state = DISK_ACTIVE;
    
    return RAID_SUCCESS;
}

raid_result_t close_disk(disk_t **disk) {
    assert(disk != NULL);
    assert(*disk != NULL);

    if ((*disk)->fd >= 0) {
        close((*disk)->fd);
    }
    
    free((*disk)->path);
    free(*disk);
    *disk = NULL;

    return RAID_SUCCESS;
}

uint64_t calc_blocks_per_stripe(uint64_t chunk_size, uint32_t num_disks){
    return chunk_size * num_disks;
}

uint64_t get_stripe_idx(uint64_t ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(config->pbs_per_stripe > 0);
    assert(config->lbs_per_stripe > 0);

    return bt == LBA ? ba / config->lbs_per_stripe : ba / config->pbs_per_stripe;
}

uint64_t get_chunk_idx(uint64_t ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(config->chunk_size > 0);

    return ba / config->chunk_size;
}

uint32_t get_disk_idx(uint64_t ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(config-> num_disks > config->num_redundancy_disks);

    uint64_t b_chunk_idx = get_chunk_idx(ba, config, bt);
    uint32_t num_disks = (bt == LBA) ? (config->num_disks - config->num_redundancy_disks) : config->num_disks;

    return (uint32_t)(b_chunk_idx % num_disks);
}

uint64_t get_stripe_offset(uint64_t ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(config->pbs_per_stripe > 0);
    assert(config->lbs_per_stripe > 0);

    return bt == LBA ? ba % config->lbs_per_stripe : ba % config->pbs_per_stripe;
}

uint64_t get_chunk_offset(uint64_t ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(config->chunk_size > 0);

    return ba % config->chunk_size;
}

uint64_t calc_stripe_span(uint64_t start_ba, uint64_t end_ba, const raid_config_t *config, block_address_t bt){
    assert(config != NULL);
    assert(start_ba <= end_ba);

    uint64_t start_stripe_idx = get_stripe_idx(start_ba, config, bt);
    uint64_t end_stripe_idx = get_stripe_idx(end_ba, config, bt);
    
    return (end_stripe_idx - start_stripe_idx) + 1;
}

raid_result_t get_disk_offset(uint64_t pba, const raid_config_t* config, disk_offset_t* disk_offset) {
    assert(config != NULL);
    assert(disk_offset != NULL);
    assert(config->chunk_size > 0);

    disk_offset->id = get_disk_idx(pba, config);

    // disk level pba offset
    uint64_t stripe_idx = get_stripe_idx(pba, config);
    uint64_t pb_chunk_offset = get_chunk_offset(pba, config);
    disk_offset->offset = (stripe_idx * config->chunk_size) + pb_chunk_offset;

    return RAID_SUCCESS;
}

raid_result_t write_blocks(disk_t *disk, uint64_t disk_pb_offset, size_t num_blocks, const void *buffer){
    assert(disk != NULL);
    assert(buffer != NULL);
    assert(num_blocks != 0);
    assert(disk->fd >= 0);
    assert(disk_pb_offset + num_blocks <= disk->num_pbs && "RAID Core attempted Out-of-Bounds Write!");

    off_t byte_offset = (off_t)(disk_pb_offset * RAID_BLOCK_SIZE);
    size_t total_bytes = num_blocks * RAID_BLOCK_SIZE;

    ssize_t res = pwrite(disk->fd, buffer, total_bytes, byte_offset);
    if (res < 0 || (size_t)res != total_bytes){
        perror("RAID HAL: Physical Write Failed");
        disk->state = DISK_FAILED;
        return RAID_ERR_IO;
    }

    return RAID_SUCCESS;
}

raid_result_t read_blocks(disk_t* disk, uint64_t disk_pb_offset, size_t num_blocks, void* buffer) {
    assert(disk != NULL);
    assert(disk->fd >= 0);
    assert(buffer != NULL);
    assert(num_blocks > 0);
    assert(disk_pb_offset + num_blocks <= disk->num_pbs && "HAL: Out-of-bounds read attempt!");

    size_t total_bytes = num_blocks * RAID_BLOCK_SIZE;
    off_t byte_offset = (off_t)(disk_pb_offset * RAID_BLOCK_SIZE);

    ssize_t result = pread(disk->fd, buffer, total_bytes, byte_offset);

    // result <= 0 handles both system errors and unexpected EOF
    if (result <= 0 || (size_t)result != total_bytes) {
        if (result < 0) perror("RAID HAL: Physical Read Failure");
        disk->state = DISK_FAILED;
        return RAID_ERR_IO;
    }

    return RAID_SUCCESS;
}
