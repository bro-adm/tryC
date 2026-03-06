#ifndef RAID_CORE
#define RAID_CORE

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "tryC/defs.h"
#include "tryC/hal.h"

raid_result_t core_init_raid_config(raid_config_t **config, raid_version_t version, raid_level_t level, uint32_t num_disks, uint64_t chunk_size);

typedef struct raid_ctx {
    raid_config_t *config;

    disk_t** disks;         // Array of physical disks
    uint64_t total_lbs;    // Total logical blocks available to user

    void* workspace;       // Pre-allocated memory for XOR/Parity math
} raid_ctx_t;

raid_result_t core_init_raid_ctx(raid_ctx_t **ctx, raid_config_t *config, const char** disk_paths);

/* --- Mappings/Geometry --- */

// mirroing of raid 1 is not considered parity
typedef struct {
    uint32_t disk_ids[2]; // Supports up to 2 parity disks (RAID 6)
    uint32_t count;       // Will be 1 for RAID 4/5, 2 for RAID 6
} redundancy_disks_t;

void core_get_redundancy_disks(const raid_ctx_t *ctx, uint64_t stripe_index, redundancy_disks_t* redundancy_disks);

// must remeber that the given user_lba is already abstarcted to the data disks
uint64_t core_lba_to_pba(const raid_ctx_t *ctx, uint64_t lba);

// Returns how many stripes a request spans across
uint64_t core_calc_stripe_span(const raid_ctx_t *ctx, uint64_t lba, size_t count);

/* --- Writes --- */

// Path A: The fastest. User provided exactly one full stripe of data.
raid_result_t core_write_full_stripe(raid_ctx_t* ctx, uint64_t stripe_index, const void* buffer);

// Path B: Read-Modify-Write (RMW). 
// Best for small writes. Reads OLD data and OLD parity to update.
raid_result_t core_write_rmw(raid_ctx_t* ctx, uint64_t lba, size_t count, const void* buffer);

// Path C: Reconstruct Write (RCW). 
// Best for large partial writes. Reads the "other" data blocks to compute parity.
raid_result_t core_write_rcw(raid_ctx_t* ctx, uint64_t lba, size_t count, const void* buffer);


/* --- Recovery & Integrity --- */

// Recovers a missing block from a degraded RAID (RAID 1 mirror or RAID 5 XOR)
raid_result_t core_recover_lb(raid_ctx_t* ctx, uint32_t failed_disk_id, uint64_t disk_offset, void* out_buffer);

// maintainence run -> uses the recover lb if needed and can be triggered in safe mode (fix_errors=false) for sys admins
raid_result_t core_scrub_stripe(raid_ctx_t* ctx, uint64_t stripe_index, bool fix_errors);


/* --- Core --- */

raid_result_t core_read(raid_ctx_t* ctx, uint64_t lba, size_t count, void* buffer);
raid_result_t core_write(raid_ctx_t* ctx, uint64_t lba, size_t count, const void* buffer);

#endif // !RAID_CORE
