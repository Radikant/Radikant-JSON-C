#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strcmp
#include <math.h>   // For NAN, INFINITY
#include "rjson.h"

// ANSI Color codes
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

void assert_true(int condition, const char *test_name)
{
    if (condition)
    {
        printf("%s[PASS]%s %s\n", GREEN, RESET, test_name);
        tests_passed++;
    }
    else
    {
        printf("%s[FAIL]%s %s\n", RED, RESET, test_name);
        tests_failed++;
    }
}

void assert_false(int condition, const char *test_name)
{
    assert_true(!condition, test_name);
}

int main()
{
    printf("=== Starting Hardened JSON Encoding Tests ===\n");

    // TEST 1: Serialization Stack Exhaustion
    // Manually construct a deep tree and try to serialize it.
    {
        printf("\n--- Test: Stack Exhaustion ---\n");
        rjson_value *root = rjson_array_new();
        rjson_value *current = root;
        int depth = 600;

        for (int i = 0; i < depth; i++)
        {
            rjson_value *next = rjson_array_new();
            rjson_array_add(current, next);
            current = next;
        }

        char *out = NULL;
        int result = rjson_serialize(root, &out, NULL);

        assert_true(result == -1, "Should fail gracefully on deep recursion");
        assert_true(out == NULL, "Output should be NULL on failure");

        rjson_free(root);
    }

    // TEST 2: NaN and Infinity
    // JSON does not support NaN or Infinity. Serialization must fail.
    {
        printf("\n--- Test: NaN / Infinity ---\n");

        rjson_value *val_nan = rjson_number_new(NAN);
        char *out = NULL;
        assert_true(rjson_serialize(val_nan, &out, NULL) == -1, "Should reject NaN");
        rjson_free(val_nan);

        rjson_value *val_inf = rjson_number_new(INFINITY);
        assert_true(rjson_serialize(val_inf, &out, NULL) == -1, "Should reject Infinity");
        rjson_free(val_inf);
    }

    // TEST 3: Control Character Escaping
    // \n, \t, etc. must be escaped.
    {
        printf("\n--- Test: Control Character Escaping ---\n");
        rjson_value *val = rjson_string_new("Line\nBreak\tTab");
        char *out = NULL;
        if (rjson_serialize(val, &out, NULL) == 0)
        {
            // Expected: "Line\nBreak\tTab" (literal backslashes)
            const char *expected = "\"Line\\nBreak\\tTab\"";
            assert_true(strcmp(out, expected) == 0, "Should escape \\n and \\t");
            free(out);
        }
        else
        {
            assert_true(0, "Serialization failed");
        }
        rjson_free(val);
    }

    // TEST 4: Extended Control Characters
    // Bytes < 0x20 must be escaped as \u00XX
    {
        printf("\n--- Test: Extended Control Escaping ---\n");
        // ASCII 0x01 (Start of Heading)
        rjson_value *val = rjson_string_new("\x01");
        char *out = NULL;
        if (rjson_serialize(val, &out, NULL) == 0)
        {
            const char *expected = "\"\\u0001\"";
            assert_true(strcmp(out, expected) == 0, "Should escape 0x01 as \\u0001");
            free(out);
        }
        else
        {
            assert_true(0, "Serialization failed");
        }
        rjson_free(val);
    }

    // TEST 5: Quote and Backslash Escaping
    {
        printf("\n--- Test: Quote and Backslash Escaping ---\n");
        rjson_value *val = rjson_string_new("Quote: \" Backslash: \\");
        char *out = NULL;
        if (rjson_serialize(val, &out, NULL) == 0)
        {
            const char *expected = "\"Quote: \\\" Backslash: \\\\\"";
            assert_true(strcmp(out, expected) == 0, "Should escape quotes and backslashes");
            free(out);
        }
        else
        {
            assert_true(0, "Serialization failed");
        }
        rjson_free(val);
    }

    // TEST 6: UTF-8 Passthrough
    // Emojis should pass through unescaped (valid JSON).
    {
        printf("\n--- Test: UTF-8 Passthrough ---\n");
        rjson_value *val = rjson_string_new("🔥");
        char *out = NULL;
        if (rjson_serialize(val, &out, NULL) == 0)
        {
            const char *expected = "\"🔥\"";
            assert_true(strcmp(out, expected) == 0, "Should preserve UTF-8 characters");
            free(out);
        }
        else
        {
            assert_true(0, "Serialization failed");
        }
        rjson_free(val);
    }

    printf("\n=== Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}