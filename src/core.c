#include "tryC/core.h"
#include "tryC/hal.h"
#include "tryC/sb.h"
#include "tryC/defs.h"
#include "tryC/helpers.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

raid_result_t core_init_raid_config(raid_config_t **config, raid_version_t version, raid_level_t level, uint32_t num_disks, uint64_t chunk_size){
    assert(config != NULL);
    if (num_disks == 0 || chunk_size == 0) return RAID_ERR_INVALID_VAL;

    // 1. Validate the Enum and set Redundancy Count
    uint32_t redundancy_count = 0;
    switch (level) {
        case RAID_0:
            redundancy_count = 0;
            break;
        case RAID_1:
            // Mirroring requires exactly 2 disks in this simple implementation
            if (num_disks != 2) return RAID_ERR_INVALID_VAL;
            redundancy_count = 1;
            break;
        case RAID_4:
        case RAID_5:
            if (num_disks < 3) return RAID_ERR_INVALID_VAL;
            redundancy_count = 1;
            break;
        // case RAID_6:
        //     if (num_disks < 4) return RAID_ERR_INVALID_VAL;
        //     redundancy_count = 2;
        //     break;
        default:
            // This catches any invalid enum values passed as integers
            return RAID_ERR_INVALID_VAL; 
    }

    assert(redundancy_count < num_disks);

    // 2. Allocation
    raid_config_t *config_p = (raid_config_t *)calloc(1, sizeof(raid_config_t));
    if (config_p == NULL) return RAID_ERR_NOMEM;

    // 3. Populate
    config_p->version = version;
    config_p->level = level;
    config_p->num_disks = num_disks;
    config_p->chunk_size = chunk_size;
    config_p->num_redundancy_disks = redundancy_count;

    // 4. Geometry Math
    config_p->pbs_per_stripe = calc_blocks_per_stripe(chunk_size, num_disks);
    config_p->lbs_per_stripe = calc_blocks_per_stripe(chunk_size, num_disks - redundancy_count);

    *config = config_p;
    return RAID_SUCCESS;
}

raid_result_t core_init_raid_ctx(raid_ctx_t **ctx, raid_config_t *config, const char** disk_paths){
    assert(ctx != NULL);
    assert(config != NULL);
    assert(disk_paths != NULL);

    // ctx alloc
    raid_ctx_t* ctx_p = (raid_ctx_t *)calloc(1, sizeof(raid_ctx_t));
    if (ctx_p == NULL) return RAID_ERR_NOMEM;
    
    // set config
    ctx_p->config = config;

    // disks alloc
    ctx_p->disks = (disk_t **)calloc(config->num_disks, sizeof(disk_t*));
    if (ctx_p->disks == NULL){
        free(ctx_p);
        return RAID_ERR_NOMEM;
    }

    // disks + lbs info init and set
    uint64_t min_pbs = UINT64_MAX;
    for (uint32_t i = 0; i < config->num_disks; i++){
        raid_result_t res = init_disk(&ctx_p->disks[i], disk_paths[i], i);
        if (res != RAID_SUCCESS) {
            for (uint32_t j = 0; j < i; j++){
                close_disk(&ctx_p->disks[j]);
            }
            free(ctx_p->disks);
            free(ctx_p);
            return res;
        }

        if (ctx_p->disks[i]->num_pbs < min_pbs) min_pbs = ctx_p->disks[i]->num_pbs;
    }

    // superblock chunks are per disk -- vertical == stripe
    uint64_t num_stripes = min_pbs / config->chunk_size;
    ctx_p->total_lbs = (num_stripes - SUPERBLOCK_CHUNK_SIZE)* config->lbs_per_stripe;

    // workspace
    ctx_p->workspace = malloc(config->pbs_per_stripe * RAID_BLOCK_SIZE);
    if (ctx_p->workspace == NULL){
        for (uint32_t i = 0; i < config->num_disks; i++){
            close_disk(&ctx_p->disks[i]);
        }
        free(ctx_p->disks);
        free(ctx_p);
        return RAID_ERR_NOMEM;
    }

    *ctx = ctx_p;
    return RAID_SUCCESS;
}

void core_get_redundancy_disks(const raid_ctx_t *ctx, uint64_t stripe_index, redundancy_disks_t *redundancy_disks){
    assert(ctx != NULL);
    assert(redundancy_disks != NULL);
    assert(ctx->config->level == RAID_4 || ctx->config->level == RAID_5);

    switch (ctx->config->level) {
        case RAID_4:
            redundancy_disks->count = 1;
            redundancy_disks->disk_ids[0] = ctx->config->num_disks - 1;
            break;
        case RAID_5:
            redundancy_disks->count = 1;
            redundancy_disks->disk_ids[0] = (ctx->config->num_disks - 1) - (stripe_index % ctx->config->num_disks);
            break;
        default:
            assert(0 && "Unreachable RAID level in redundancy lookup");
            __builtin_unreachable();
    }
}

uint64_t core_lba_to_pba(const raid_ctx_t *ctx, uint64_t lba){
    assert(ctx != NULL);
    raid_config_t *cfg = ctx->config;

    uint64_t stripe_idx = get_stripe_idx(lba, cfg, LBA);
    uint32_t logical_disk_idx = get_disk_idx(lba, cfg, LBA);
    uint64_t block_chuck_offset = get_chunk_offset(lba, cfg, LBA);

    uint32_t physical_disk_id = logical_disk_idx; // default val

    if (cfg->level == RAID_4 || cfg->level == RAID_5){
        redundancy_disks_t redundancy_disks = {0};
        core_get_redundancy_disks(ctx, stripe_idx, &redundancy_disks);
        assert(redundancy_disks.count == 1);

        if (logical_disk_idx >= redundancy_disks.disk_ids[0]){
            physical_disk_id++;
        }
    }
    else if (cfg->level == RAID_1){
        physical_disk_id = 0;
    }

    uint64_t pba = (stripe_idx * cfg->pbs_per_stripe) + 
        (uint64_t)physical_disk_id * cfg->chunk_size + block_chuck_offset;

    // addition of superblock stripe to pba
    return pba + cfg->pbs_per_stripe*SUPERBLOCK_CHUNK_SIZE;
}

uint64_t core_calc_stripe_span(const raid_ctx_t *ctx, uint64_t lba, size_t count){
    assert(ctx != NULL);
    if (count == 0 ) return 0;

    uint64_t end_lba = lba + count - 1;
    return calc_stripe_span(lba, end_lba, ctx->config, LBA);
}





































uint64_t core_calc_stripe_span(uint64_t lba, size_t count, const raid_ctx_t *ctx){
    assert(ctx != NULL);

    if (count == 0) return 0;

    uint64_t end_lba = lba + count - 1;

    uint64_t phys_start_lba = core_user_to_physical_lba(lba, ctx->redundancy_disks_count, &ctx->config);
    uint64_t phys_end_lba = core_user_to_physical_lba(end_lba, ctx->redundancy_disks_count, &ctx->config);
    
    return calc_stripe_span(phys_start_lba, phys_end_lba, &ctx->config);
}

