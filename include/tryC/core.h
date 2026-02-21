#ifndef RAID_CORE
#define RAID_CORE

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "tryC/hal.h"

typedef struct raid_ctx {
    raid_config_t config;
    disk_t* disks;         // Array of physical disks
    uint64_t total_lbs;    // Total logical blocks available to user
    void* workspace;       // Pre-allocated memory for XOR/Parity math
} raid_ctx_t;

/* --- Mappings/Geometry --- */

typedef struct {
    uint32_t disk_ids[2]; // Supports up to 2 parity disks (RAID 6)
    uint32_t count;       // Will be 1 for RAID 4/5, 2 for RAID 6
} parity_disks_t;

parity_disks_t core_get_parity_disks(uint64_t stripe_index, const raid_config_t* config);

// Returns how many stripes a request spans across
uint64_t core_calculate_stripe_count(uint64_t lba, size_t count, const raid_config_t* config);


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


/* --- Math Primitives (Internal) --- */

// Pure memory XOR: target = target ^ source
void core_xor_blocks(void* target, const void* source, size_t len);

// Checksum calculation for a block of data
uint32_t core_calculate_checksum(const void* buffer, size_t len);


/* --- Core --- */

raid_result_t core_read(raid_ctx_t* ctx, uint64_t lba, size_t count, void* buffer);
raid_result_t core_write(raid_ctx_t* ctx, uint64_t lba, size_t count, const void* buffer);

#endif // !RAID_CORE
