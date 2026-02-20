#include "tryC/bitops.h"
#include <stddef.h>

void swap(void* a, void* b, size_t len) {
    unsigned char *byte_a = (unsigned char *)a;
    unsigned char *byte_b = (unsigned char *)b;

    for (size_t i=0; i<len; i++) {
        byte_a[i] = byte_a[i]^byte_b[i];
        byte_b[i] = byte_a[i]^byte_b[i]; // original_byte_a[i]^byte_b[i]^byte_b[i] = original_byte_a[i] -> swap
        byte_a[i] = byte_a[i]^byte_b[i]; // original_byte_a[i]^original_byte_b[i] ^ original_byte_a[i]^original_byte_b[i]^original_byte_b[i] = original_byte_b[i] -> swap
    }
}
