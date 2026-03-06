/*
 * Low-level utility functions
 * Bitwise operations (XOR for parity), buffer manipulation,
 * and Galois Field math (for future RAID 6 support)
 */

#ifndef RAID_HELPERS
#define RAID_HELPERS

#include <stddef.h>

#define BYTES_TO_BLOCKS(b) (((b) + RAID_BLOCK_SIZE - 1) / RAID_BLOCK_SIZE)
#define IS_POWER_OF_2(x) (((x) & ((x) - 1)) == 0)

#if defined(__linux__) || defined(__CYGWIN__)
    #include <endian.h>
#elif defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define htole32(x) OSSwapHostToLittleInt32(x)
    #define le32toh(x) OSSwapLittleToHostInt32(x)
    #define htole64(x) OSSwapHostToLittleInt64(x)
    #define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(_WIN32)
    #include <winsock2.h>
    // Windows is Little Endian, so htole is a no-op
    #define htole32(x) (x)
    #define le32toh(x) (x)
    #define htole64(x) (x)
    #define le64toh(x) (x)
#else
    #warning "Unknown platform: assuming Little Endian for RAID metadata"
    #define htole32(x) (x)
    #define le32toh(x) (x)
    #define htole64(x) (x)
    #define le64toh(x) (x)
#endif

// Pure memory XOR: target = target ^ source
void core_xor_blocks(void* target, const void* source, size_t len);

// Checksum calculation for a block of data
uint32_t core_calculate_checksum(const void* buffer, size_t len);



#endif // !RAID_HELPERS
