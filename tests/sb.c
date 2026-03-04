#include <criterion/criterion.h>
#include "tryC/sb.h"
#include "tryC/hal.h"
#include <unistd.h>

#define SB_TEST_PATH "sb_block.bin"
#define BIG_DISK_SIZE (1024 * 1024) // 1MB

static disk_t *sb_disk = NULL;

void sb_suite_setup(void) {
    FILE* f = fopen(SB_TEST_PATH, "wb");
    if (f) {
        ftruncate(fileno(f), BIG_DISK_SIZE); 
        fclose(f);
    }
    raid_result_t res = init_disk(&sb_disk, SB_TEST_PATH, 0);

    cr_assert_eq(res, RAID_SUCCESS, "Setup failed: Could not initialize sb_disk");
}

void sb_suite_teardown(void) {
    if (sb_disk) close_disk(&sb_disk);
    unlink(SB_TEST_PATH);
}

TestSuite(sb_suite, .init = sb_suite_setup, .fini = sb_suite_teardown);

