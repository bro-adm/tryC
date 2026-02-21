#ifndef RAID_DEFS
#define RAID_DEFS

#include <stdint.h>


/**
 * Set of common raid config info for actions such as getting disk id + offset (becasue differs according to raid level and the chunk size and of cousre the number of dsik availble in general)
 * This is regardless of a raid_ctx_t that includes state of disks and so and so...
 *
 * @chunk_size: chunk is a group of logical blocks, so its size is measured in logical blocks amount and this is const
 * uint32_t becuase its better for cpu ops even tough its allows for much biggernumbers then chunk_size will actually need to represent.
*/

typedef enum {
    RAID_V1
} raid_version_t;

typedef enum {RAID_0, RAID_1, RAID_4, RAID_5} raid_level_t; // Changes the meaning of strips/chunks accordfing to their position and disk...

typedef struct {
    raid_level_t level;
    raid_version_t version;
    uint32_t num_disks;
    uint32_t chunk_size;
} raid_config_t;

#define SECTOR_SIZE 512 // smallest read/write availble by hardware -> emulation from modern 4096 byte sectors
#define RAID_BLOCK_SIZE 512 // smallest raid read/write availble

typedef enum { DISK_ACTIVE, DISK_FAILED, DISK_REBUILDING, DISK_MISSING } disk_state_t; 

typedef enum {
    RAID_SUCCESS          =  0,
    RAID_ERR_IO           = -1, 
    RAID_ERR_SIGNATURE    = -2, 
    RAID_ERR_CHECKSUM     = -3, 
    RAID_ERR_INVALID_VAL  = -4, 
    RAID_ERR_NOMEM        = -5,
    RAID_ERR_INTERNAL
} raid_result_t;

#endif // !RAID_DEFS
