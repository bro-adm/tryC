/*
 * Public API for RAID volume operations
 * Provides user-facing read/write interface and request validation
 *
 * Main API manages allocations and desires only a pointer slot via ** instead fo common *
 * Main API only talks in Logical blocks (raid blocks)
 *
 */

#ifndef RAID_LIB
#define RAID_LIB

#include <stddef.h>
#include <stdint.h>

#include "tryC/defs.h"

typedef struct raid_ctx raid_ctx_t; // opaque type for public faicng api headers

// initalizes new raid with new superblocks
raid_result_t raid_format(raid_ctx_t** raid_ctx, const raid_config_t* raid_config, const char* disk_paths[]);

/* 
 * initalizes raid from superblocks, validates expected set of disks found
 * Option A: Open the first RAID found in these paths
 * Option B: Open a specific RAID by its UUID
*/
raid_result_t raid_open(raid_ctx_t** ctx, const char* disk_paths[], size_t num_paths);
raid_result_t raid_open_by_uuid(raid_ctx_t** ctx, const uint8_t uuid[16], const char* disk_paths[], size_t num_paths);

// closes fds, frees allocated memory (makes sure null invalidate the ctx)
raid_result_t raid_close(raid_ctx_t** raid_ctx);

// IO
raid_result_t raid_read(raid_ctx_t* ctx, uint64_t lba_start, size_t lb_count, void* buffer);
raid_result_t raid_write(raid_ctx_t* ctx, uint64_t lba_start, size_t lb_count, const void* buffer);

// Debug
uint64_t raid_get_total_lbs(const raid_ctx_t* ctx);

#endif // !RAID_LIB
