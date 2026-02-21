/*
 * Disk topology and mapping layer
 * Maps LBAs to physical disk offset
 * Includes disk structs for runtime
 *
 * input: logical block address (lba) + raid level
 * output: physical disk id + disk offset in blocks
 *
 * lba -> chunk id -> disk + offeset
 *
 * like the GPS: gets the known named address (lba) along with the roads lanes and rules (disk count and raid level). returns the program known location
 *
 * Concpets:
 *  Num Blocks -> either uint64_t or size_t beucase for one its the general size availble and cna be big while the other is for how much to read/write and that needs to be a size that fits in memort so size_t is the correct option there
 *  fd -> file descriptor (in linux everything is a file) and this allows continous atomic actions via pread/pwrite without reopening or reseeking the offset...  negative value is error, positive is non changing and represent the disk id for further ops
 * 
 */

#ifndef RAID_HARDWARE_ABSTRACTION_LAYER
#define RAID_HARDWARE_ABSTRACTION_LAYER 

#include <stddef.h>
#include <stdint.h>

#include "tryC/defs.h"

// Physical Disk Metadata - forward declared in metadata
typedef struct disk {
    char* path;
    uint32_t id;
    uint64_t num_lbs;
    disk_state_t state;
    int fd;
} disk_t;

// inits pre allocated empty disk_t insatnce
raid_result_t init_disk(disk_t** disk, const char* path, uint32_t id);

raid_result_t check_disk_availability(disk_t* disk);

raid_result_t close_disk(disk_t** disk);

// GPS result
typedef struct {
    uint32_t id;
    uint64_t offset;
} disk_offset_t;

// initializes disk offset from lba + raid config
raid_result_t get_disk_offset(uint64_t lba, const raid_config_t* config, disk_offset_t* disk_offset);

// intended to be used after mapping of lba via get_disk_loc  -> dumb write blocks on a single disk, the method that uses these will need to be samrt and decide when and where to write the next lba...
raid_result_t write_blocks(disk_t* disk, uint64_t disk_lb_offset, size_t num_blocks, const void* buffer);
raid_result_t read_blocks(disk_t* disk, uint64_t disk_lb_offset, size_t num_blocks, void* buffer);

#endif // !RAID_HARDWARE_ABSTRACTION_LAYER
