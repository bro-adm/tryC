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
    disk_p->num_lbs = (uint64_t)byte_size / RAID_BLOCK_SIZE;

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

