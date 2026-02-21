/*
 * Superblock and metadata management
 * Handles per-disk metadata: disk identity, RAID member UUID, state, configuration
 *
 * size matters. it differs on machines and so the types used must be hardly set adn defined instead of enums ar just ints...
 * now we also have machines that do differnt Endian logics thus having a Endianess issue for types like uint32 or uint64.
 * then we have uint8 (a single byte) and we cna do an array of thta and that will always be the same order.
 * why not use array of uin8 always? machines are efficient in reading 32 and 64 bits at a time... making them read one byte at a time is slower...
 * 
 * solution for endianess is endian conversion macros like htole32
 *
 * regarding uuid its mostly 16 bytes or 8*16= 128 bits. meaning uint8[16] or uint64[2] etc...
 * we said uint64 was faster to read then a sinble byte at a time...
 * but uint64 requres 8 bytes alligned on some architectures or the block starts at an odd memory address then it can cause issues.
 * using the uint8 (a single byte) does not require alligned bytes.
 *
 * we can do the work and make sure our struct is padded and becuase its the superblock at the begginign it will be a maanged solution to the issue.
 * but then most uuid libs provide uint8[16] so we will go with that for here...
 *
 * using uint32 and uint64 is less about future proffing but just making the written code be the used code instead of the comilers being the ones to pad the structs itself.
 *
 * includes classic concepts of capacity and unit size for container object
 *
 */

#ifndef RAID_SUPERBLOCK
#define RAID_SUPERBLOCK

#include <stddef.h>
#include <stdint.h>

#include "tryC/defs.h"
#include "tryC/helpers.h"

typedef struct disk disk_t; // opaque type for disk_t declared in partition.h

#define RAID_SIGNATURE 0x52414944 // RAID in big endian

#define SUPERBLOCK_OFFSET 0
#define SUPERBLOCK_SIZE 1

/* 
 * should take one logical block - raid block
 * cpu friendly - field alignment - 4 and 8 byte allignment
 * cache friendly - struct size - powers of 2 - 64,...
*/  
typedef struct {
    uint32_t raid_signature; // 0-4
    uint32_t raid_version;   // 4-8
    uint8_t  raid_uuid[16];  // Alligned to 8-byte boundary 8-24
    uint32_t raid_level;     // 24-28

    uint32_t disk_id;        // 28-32
    uint32_t num_disks;      //32-36
    uint32_t chunk_size;     // 36-40
    
    uint64_t disk_capacity;  // Aligned to 8-byte boundary 40-48
    uint32_t checksum;       // 48-52
    
    uint32_t _padding[3];      // 52-64
                            // 
} __attribute__((packed)) superblock_t;

// --- Safety Checks ---
_Static_assert(sizeof(superblock_t) <= RAID_BLOCK_SIZE, "Logic error: struct exceeds 512 bytes");
_Static_assert(IS_POWER_OF_2(sizeof(superblock_t)), "Size must be power of 2");
_Static_assert(sizeof(superblock_t) % 8 == 0, "Size must be 8-byte aligned");
_Static_assert(offsetof(superblock_t, disk_capacity) % 8 == 0, "64-bit field misaligned");
_Static_assert(offsetof(superblock_t, raid_uuid) % 8 == 0, "UUID field misaligned for 64-bit ops");

// superblock will be on the stack so pass a pointer and not a ** that requires internlmalloc
raid_result_t init_sb_t(const disk_t* disk, const raid_config_t* config, const uint8_t uuid[16], superblock_t* sb); 

raid_result_t write_sb(disk_t* disk, const superblock_t* sb);

raid_result_t read_sb(disk_t* disk, superblock_t* sb);

void print_sb(const superblock_t* sb);

#endif // !RAID_SUPERBLOCK
