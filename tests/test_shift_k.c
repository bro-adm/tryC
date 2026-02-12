#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <string.h>

#include "tryC/shift_k.h"

// Define the struct locally or globally, doesn't matter for this approach
typedef struct {
    const char *initial;
    int k;
    const char *expected;
} ShiftKTestCase;

Test(shift_k_tests, working_cases_loop) {
    // 1. Define your cases right here. No static/global linker magic needed.
    ShiftKTestCase cases[] = {
        {"abc", 1, "bcd"},
        {"ABC", 1, "BCD"},

        // Wrapping Forwards
        {"xyz", 1, "yza"},
        {"XYZ", 1, "YZA"},
        {"z", 1, "a"},
        {"Z", 1, "A"},

        // Wrapping Backwards (assuming your _modulo handles negatives)
        {"a", -1, "z"}, 
        {"A", -1, "Z"},

        // Large K (Modulo arithmetic)
        {"abc", 26, "abc"}, // Full cycle
        {"abc", 27, "bcd"}, // 26 + 1

        // Non-alphabetic preservation
        {"123", 5, "123"},
        {"a-b-c", 1, "b-c-d"},
        {"Hello World!", 1, "Ifmmp Xpsme!"} 

    };

    size_t num_cases = sizeof(cases) / sizeof(ShiftKTestCase);

    // 2. Iterate manually. This is standard C and cannot fail randomly.
    for (size_t i = 0; i < num_cases; ++i) {
        ShiftKTestCase *params = &cases[i];

        char buffer[256];
        // Safe copy
        strncpy(buffer, params->initial, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        shift_k(buffer, params->k);

        // 3. Use cr_expect (instead of cr_assert) so if one fails, the loop continues
        // and you see all failures.
        cr_expect_str_eq(buffer, params->expected, 
            "Failed on index %zu:\n\tInput:    '%s'\n\tK:        %d\n\tExpected: '%s'\n\tGot:      '%s'", 
            i, params->initial, params->k, params->expected, buffer);
    }
}

// Keep the edge cases as standalone tests
Test(shift_k_tests, handles_null_pointer) {
    shift_k(NULL, 0);
    cr_assert(true);
}

Test(shift_k_tests, handles_empty_string) {
    char input[] = "";
    shift_k(input, 5);
    cr_assert_str_eq(input, "", "The empty string should remain unchanged");
}
