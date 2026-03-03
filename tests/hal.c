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

Test(hal_suite, val_init) {
    cr_expect_eq(active_disk->num_lbs, 2048, "Disk should have exactly 2048 blocks");
    cr_expect_eq(active_disk->id, 0);
}

Test(hal_suite, availability_active) {
    cr_expect_eq(active_disk->state, DISK_ACTIVE, "Expected initial disk state to be active for this test to run"); // pretest expectation

    // 1. Probing a healthy disk should succeed and keep it ACTIVE
    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS);
    cr_assert_eq(active_disk->state, DISK_ACTIVE);
}

Test(hal_suite, availability_missing) {
    int original_fd = active_disk->fd;
    active_disk->fd = -1;

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS); // Function returns success because it handled the check
    cr_assert_eq(active_disk->state, DISK_MISSING, "Disk with fd -1 should be marked MISSING");

    active_disk->fd = original_fd;
}

Test(hal_suite, availability_io_failure) {
    int real_fd = active_disk->fd;
    close(real_fd); 

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_ERR_IO, "Probe should fail if FD is closed externally");
    cr_assert_eq(active_disk->state, DISK_FAILED, "Disk should move to FAILED state on probe failure");

    active_disk->fd = -1; 
}

Test(hal_suite, availability_rebuild_preservation) {
    active_disk->state = DISK_REBUILDING;

    raid_result_t res = check_disk_availability(active_disk);
    cr_assert_eq(res, RAID_SUCCESS);
    cr_assert_eq(active_disk->state, DISK_REBUILDING, "Check should not overwrite REBUILDING state");
}

