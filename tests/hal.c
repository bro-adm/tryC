#include <criterion/criterion.h>
#include "tryC/hal.h"
#include <stdio.h>
#include <unistd.h>

#define TEST_DISK_PATH "hal_fixture.bin"
#define TEST_DISK_SIZE (1024 * 1024) 

static disk_t *active_disk = NULL;

void hal_suite_setup(void) {
    FILE* f = fopen(TEST_DISK_PATH, "wb");
    if (f) {
        ftruncate(fileno(f), TEST_DISK_SIZE);
        fclose(f);
    }

    raid_result_t res = init_disk(&active_disk, TEST_DISK_PATH, 0);
    cr_assert_eq(res, RAID_SUCCESS, "HAL Setup: Failed to initialize disk fixture");
}

void hal_suite_teardown(void) {
    if (active_disk) {
        close_disk(&active_disk); // close disk is safe and doent double close...
    }
    unlink(TEST_DISK_PATH);
}

TestSuite(hal_suite, .init = hal_suite_setup, .fini = hal_suite_teardown);

