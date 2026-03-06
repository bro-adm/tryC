#include "tryC/helpers.h"

#include <assert.h>

void core_xor_blocks(void* target, const void* source, size_t len) {
    assert(target != NULL && source != NULL);
    assert(len % 8 == 0 && "XOR blocks must be 8-byte aligned for performance");

    uint64_t* t = (uint64_t*)target;
    const uint64_t* s = (const uint64_t*)source;
    size_t num_words = len / 8;

    for (size_t i = 0; i < num_words; i++) {
        t[i] ^= s[i];
    }
}

uint32_t calculate_checksum(const void* buffer, size_t len) {
    assert(buffer != NULL);
    assert(len % 4 == 0 && "Buffer must be 4-byte aligned for 32-bit checksum");

    const uint32_t* data = (const uint32_t*)buffer;
    size_t count = len / 4;
    uint32_t checksum = 0;

    for (size_t i = 0; i < count; i++) {
        checksum += data[i];
    }

    return checksum;
}
