#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strcmp
#include "rjson.h"

// ANSI Color codes for nicer output
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
    printf("=== Starting Hardened JSON Decoding Tests ===\n");

    // TEST 1: Stack Exhaustion Protection
    // We attempt to parse a JSON structure deeper than RJSON_MAX_DEPTH (512).
    // This ensures the parser fails gracefully instead of crashing the process.
    {
        printf("\n--- Test: Stack Exhaustion ---\n");
        int depth = 600;
        char *deep_json = (char *)malloc(depth * 2 + 1);
        if (deep_json)
        {
            char *p = deep_json;
            for (int i = 0; i < depth; i++)
                *p++ = '[';
            for (int i = 0; i < depth; i++)
                *p++ = ']';
            *p = '\0';

            rjson_value *val = rjson_parse(deep_json);
            assert_false(val != NULL, "Deeply nested array (600) should fail gracefully");
            if (val)
                rjson_free(val);
            free(deep_json);
        }
    }

    // TEST 2: Unicode Surrogate Pair Decoding
    // Input: "\uD83D\uDE00" (Grinning Face Emoji 😀)
    // Expected: UTF-8 bytes F0 9F 98 80
    {
        printf("\n--- Test: Unicode Surrogate Pairs ---\n");
        const char *json = "\"\\uD83D\\uDE00\"";
        rjson_value *val = rjson_parse(json);
        assert_true(val != NULL, "Should parse surrogate pair escape sequence");

        if (val && val->type == RJSON_STRING)
        {
            unsigned char *bytes = (unsigned char *)val->as.str_val;
            // Check for UTF-8 encoding of U+1F600
            int is_correct = (bytes[0] == 0xF0 && bytes[1] == 0x9F &&
                              bytes[2] == 0x98 && bytes[3] == 0x80);
            assert_true(is_correct, "Should decode to correct UTF-8 bytes (😀)");
        }
        rjson_free(val);
    }

    // TEST 3: BOM (Byte Order Mark) Handling
    // Input: EF BB BF { "a": 1 }
    // Many editors add this invisible header. A robust parser must skip it.
    {
        printf("\n--- Test: BOM Handling ---\n");
        const char *json = "\xEF\xBB\xBF{\"a\":1}";
        rjson_value *val = rjson_parse(json);
        assert_true(val != NULL, "Should ignore UTF-8 BOM at start of file");
        rjson_free(val);
    }

    // TEST 4: Strict Number Parsing (Leading Zeros)
    // Input: 01
    // RFC 8259 says integers cannot have leading zeros unless it is just "0".
    // This prevents confusion between octal and decimal.
    {
        printf("\n--- Test: Strict Number Parsing ---\n");
        const char *json = "01";
        rjson_value *val = rjson_parse(json);
        assert_false(val != NULL, "Should reject numbers with leading zeros (e.g., 01)");
        if (val)
            rjson_free(val);

        json = "0";
        val = rjson_parse(json);
        assert_true(val != NULL, "Should accept single zero");
        rjson_free(val);
    }

    // TEST 5: Unescaped Control Characters
    // Input: "Line\nBreak" (literal newline byte, not \n escape)
    // RFC 8259 requires control chars (U+0000 through U+001F) to be escaped.
    {
        printf("\n--- Test: Unescaped Control Characters ---\n");
        const char *json = "\"Line\nBreak\"";
        rjson_value *val = rjson_parse(json);
        assert_false(val != NULL, "Should reject unescaped newline in string");
        if (val)
            rjson_free(val);

        json = "\"Line\\nBreak\""; // Valid escaped
        val = rjson_parse(json);
        assert_true(val != NULL, "Should accept escaped newline");
        rjson_free(val);
    }

    // TEST 6: Trailing Commas
    // JSON does not allow trailing commas in arrays or objects.
    {
        printf("\n--- Test: Trailing Commas ---\n");
        const char* json = "[1, 2, 3,]";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject trailing comma in array");
        if (val) rjson_free(val);

        json = "{\"a\": 1,}";
        val = rjson_parse(json);
        assert_false(val != NULL, "Should reject trailing comma in object");
        if (val) rjson_free(val);
    }

    // TEST 7: Invalid Number Formats
    // Edge cases like leading plus, trailing dots, etc.
    {
        printf("\n--- Test: Invalid Number Formats ---\n");
        const char* invalid_nums[] = {
            "+1",   // Leading plus not allowed
            "1.",   // Trailing dot not allowed
            ".1",   // Leading dot not allowed
            "1e",   // Exponent without digits
            "1.e1", // Dot without fraction digits
            NULL
        };
        
        for (int i = 0; invalid_nums[i]; i++) {
            rjson_value* val = rjson_parse(invalid_nums[i]);
            if (val) {
                printf("  Failed to reject: %s\n", invalid_nums[i]);
                rjson_free(val);
                assert_false(1, "Should reject invalid number format");
            } else {
                assert_true(1, "Rejected invalid number format");
            }
        }
    }

    // TEST 8: Garbage after valid JSON
    // A parser should stop after the first valid JSON value and ensure EOF (or whitespace).
    {
        printf("\n--- Test: Garbage After JSON ---\n");
        const char* json = "{} garbage";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject content after valid JSON");
        if (val) rjson_free(val);
    }
    
    // TEST 9: Invalid Unicode Escapes
    // Short sequences, invalid hex.
    {
        printf("\n--- Test: Invalid Unicode Escapes ---\n");
        const char* json = "\"\\u123\""; // Too short
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject short \\u sequence");
        if (val) rjson_free(val);

        json = "\"\\u12GG\""; // Invalid hex
        val = rjson_parse(json);
        assert_false(val != NULL, "Should reject invalid hex in \\u sequence");
        if (val) rjson_free(val);
    }

    // TEST 10: Comments (Should Fail)
    // JSON does not support comments.
    {
        printf("\n--- Test: Comments ---\n");
        const char* json = "[1, 2 /* comment */]";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject inline comments");
        if (val) rjson_free(val);
    }

    // TEST 11: Top-level Scalars
    // RFC 8259 allows top-level scalars (strings, numbers, bools, null).
    {
        printf("\n--- Test: Top-level Scalars ---\n");
        rjson_value* val = rjson_parse("\"hello\"");
        assert_true(val != NULL && val->type == RJSON_STRING, "Should parse top-level string");
        rjson_free(val);

        val = rjson_parse("123");
        assert_true(val != NULL && val->type == RJSON_NUMBER, "Should parse top-level number");
        rjson_free(val);

        val = rjson_parse("true");
        assert_true(val != NULL && val->type == RJSON_BOOL, "Should parse top-level boolean");
        rjson_free(val);
    }

    // TEST 12: Number Overflow
    // Numbers exceeding double precision limits (approx 1.8e308) should fail.
    {
        printf("\n--- Test: Number Overflow ---\n");
        const char* json = "1e309";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject number overflow (Infinity)");
        if (val) rjson_free(val);
    }

    // TEST 13: Invalid Escapes (Extended)
    // \v, \a, \' are valid in C but NOT in JSON.
    {
        printf("\n--- Test: Invalid Escapes (Extended) ---\n");
        const char* invalid_escapes[] = {
            "\"\\v\"", // Vertical tab
            "\"\\a\"", // Alert/Bell
            "\"\\'\"", // Single quote escape (not needed/valid in JSON strings)
            "\"\\x00\"", // Hex escape (not valid in JSON, must use \u0000)
            NULL
        };
        for (int i = 0; invalid_escapes[i]; i++) {
            rjson_value* val = rjson_parse(invalid_escapes[i]);
            if (val) {
                printf("  Failed to reject: %s\n", invalid_escapes[i]);
                rjson_free(val);
                assert_false(1, "Should reject invalid escape");
            } else {
                assert_true(1, "Rejected invalid escape");
            }
        }
    }

    // TEST 14: Case Sensitivity
    // true/false/null must be lowercase.
    {
        printf("\n--- Test: Case Sensitivity ---\n");
        const char* invalid_literals[] = { "True", "FALSE", "Null", "NULL", NULL };
        for (int i = 0; invalid_literals[i]; i++) {
            rjson_value* val = rjson_parse(invalid_literals[i]);
            assert_false(val != NULL, "Should reject incorrect case literal");
            if (val) rjson_free(val);
        }
    }

    // TEST 15: Deeply Nested Objects
    {
        printf("\n--- Test: Deeply Nested Objects ---\n");
        int depth = 600;
        // Construct {"a":{"a": ... }}
        char* deep_json = (char*)malloc(depth * 6 + 1);
        if (deep_json) {
            char* p = deep_json;
            for(int i=0; i<depth; i++) {
                strcpy(p, "{\"a\":"); p += 5;
            }
            strcpy(p, "1"); p++;
            for(int i=0; i<depth; i++) *p++ = '}';
            *p = '\0';

            rjson_value* val = rjson_parse(deep_json);
            assert_false(val != NULL, "Deeply nested object (600) should fail gracefully");
            if (val) rjson_free(val);
            free(deep_json);
        }
    }

    // TEST 16: Lone Surrogates (Invalid UTF-8)
    // \uD800 without a low surrogate is invalid in strict UTF-8.
    // A hardened parser should reject this to ensure output is always valid UTF-8.
    {
        printf("\n--- Test: Lone Surrogates ---\n");
        const char* json = "\"\\uD800\"";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject lone surrogate (invalid UTF-8)");
        if (val) rjson_free(val);
    }

    // TEST 17: Null Bytes in Strings
    // JSON allows \u0000, but in C (char*), this terminates the string early.
    // A hardened C library should reject this to prevent data truncation vulnerabilities.
    {
        printf("\n--- Test: Null Bytes ---\n");
        const char* json = "\"\\u0000\"";
        rjson_value* val = rjson_parse(json);
        assert_false(val != NULL, "Should reject \\u0000 (unsafe for C-strings)");
        if (val) rjson_free(val);
    }

    // TEST 18: Duplicate Keys
    // RFC 8259 says names SHOULD be unique, but doesn't forbid duplicates.
    // The parser must not crash or leak memory.
    {
        printf("\n--- Test: Duplicate Keys ---\n");
        const char* json = "{\"a\": 1, \"a\": 2}";
        rjson_value* val = rjson_parse(json);
        assert_true(val != NULL, "Should accept duplicate keys (valid JSON)");
        
        // Optional: Check which value is preserved (usually last one wins or both kept)
        if (val) {
            rjson_value* inner = rjson_object_get_value(val, "a");
            assert_true(inner != NULL, "Should be able to retrieve key 'a'");
            rjson_free(val);
        }
    }

    // TEST 19: Keywords as Keys
    // "true", "null", "false" are valid keys in an object.
    {
        printf("\n--- Test: Keywords as Keys ---\n");
        const char* json = "{\"true\": 1, \"null\": 2, \"false\": 3}";
        rjson_value* val = rjson_parse(json);
        assert_true(val != NULL, "Should accept keywords as object keys");
        if (val) {
            rjson_value* v = rjson_object_get_value(val, "true");
            assert_true(v != NULL && v->type == RJSON_NUMBER, "Should retrieve 'true' key");
            rjson_free(val);
        }
    }

    // TEST 20: Strict Whitespace
    // JSON only allows space (0x20), tab (0x09), LF (0x0A), CR (0x0D).
    // C's isspace() allows form feed (\f) and vertical tab (\v), which are invalid in JSON.
    {
        printf("\n--- Test: Strict Whitespace ---\n");
        const char* invalid_ws[] = { "[\f]", "[\v]", NULL };
        for (int i = 0; invalid_ws[i]; i++) {
            rjson_value* val = rjson_parse(invalid_ws[i]);
            assert_false(val != NULL, "Should reject non-JSON whitespace (\\f, \\v)");
            if (val) rjson_free(val);
        }
    }

    // TEST 21: Invalid Array Structure
    // Missing commas, wrong separators.
    {
        printf("\n--- Test: Invalid Array Structure ---\n");
        const char* invalid_arrays[] = { "[1:2]", "[1 2]", NULL };
        for (int i = 0; invalid_arrays[i]; i++) {
            rjson_value* val = rjson_parse(invalid_arrays[i]);
            assert_false(val != NULL, "Should reject invalid array structure");
            if (val) rjson_free(val);
        }
    }

    // TEST 22: Invalid Object Structure
    // Missing colon, comma instead of colon.
    {
        printf("\n--- Test: Invalid Object Structure ---\n");
        const char* invalid_objects[] = { "{\"a\", 1}", "{\"a\" 1}", NULL };
        for (int i = 0; invalid_objects[i]; i++) {
            rjson_value* val = rjson_parse(invalid_objects[i]);
            assert_false(val != NULL, "Should reject invalid object structure");
            if (val) rjson_free(val);
        }
    }

    // TEST 23: Mismatched Brackets
    {
        printf("\n--- Test: Mismatched Brackets ---\n");
        const char* invalid_brackets[] = { "[}", "{]", NULL };
        for (int i = 0; invalid_brackets[i]; i++) {
            rjson_value* val = rjson_parse(invalid_brackets[i]);
            assert_false(val != NULL, "Should reject mismatched brackets");
            if (val) rjson_free(val);
        }
    }

    // TEST 24: Incomplete JSON (EOF)
    {
        printf("\n--- Test: Incomplete JSON (EOF) ---\n");
        const char* incomplete[] = { "[", "{", "{\"a\":", "[1,", NULL };
        for (int i = 0; incomplete[i]; i++) {
            rjson_value* val = rjson_parse(incomplete[i]);
            assert_false(val != NULL, "Should reject incomplete JSON");
            if (val) rjson_free(val);
        }
    }

    // TEST 25: Tricky Valid Numbers
    {
        printf("\n--- Test: Tricky Valid Numbers ---\n");
        const char* valid_nums[] = { "-0", "0e0", "0E+1", "0.0", "-0.0", NULL };
        for (int i = 0; valid_nums[i]; i++) {
            rjson_value* val = rjson_parse(valid_nums[i]);
            assert_true(val != NULL && val->type == RJSON_NUMBER, "Should accept tricky valid number");
            rjson_free(val);
        }
    }

    // TEST 26: Escaped Forward Slash
    // JSON allows \/ to escape /, but / is also valid unescaped.
    {
        printf("\n--- Test: Escaped Forward Slash ---\n");
        const char* json = "\"\\/\"";
        rjson_value* val = rjson_parse(json);
        assert_true(val != NULL, "Should accept escaped forward slash");
        if (val && val->type == RJSON_STRING) {
            assert_true(strcmp(val->as.str_val, "/") == 0, "Should decode \\/ to /");
        }
        rjson_free(val);
    }

    // TEST 27: Raw UTF-8 Input
    // The parser should pass through raw UTF-8 bytes in strings (e.g. actual emoji bytes).
    {
        printf("\n--- Test: Raw UTF-8 Input ---\n");
        // "🔥" in UTF-8 is F0 9F 94 A5
        const char* json = "\"🔥\""; 
        rjson_value* val = rjson_parse(json);
        assert_true(val != NULL, "Should accept raw UTF-8 characters in string");
        if (val && val->type == RJSON_STRING) {
            // Check bytes
            unsigned char* bytes = (unsigned char*)val->as.str_val;
            int is_correct = (bytes[0] == 0xF0 && bytes[1] == 0x9F && 
                              bytes[2] == 0x94 && bytes[3] == 0xA5);
            assert_true(is_correct, "Should preserve raw UTF-8 bytes");
        }
        rjson_free(val);
    }
    
    // TEST 28: Empty Structures
    {
        printf("\n--- Test: Empty Structures ---\n");
        rjson_value* val = rjson_parse("[]");
        assert_true(val != NULL && val->type == RJSON_ARRAY && val->as.arr_val.count == 0, "Should parse empty array");
        rjson_free(val);

        val = rjson_parse("{}");
        assert_true(val != NULL && val->type == RJSON_OBJECT && val->as.obj_val.count == 0, "Should parse empty object");
        rjson_free(val);
    }

    // TEST 29: Whitespace Torture
    {
        printf("\n--- Test: Whitespace Torture ---\n");
        const char* json = " \t \n \r [ \t \n \r 1 \t \n \r , \t \n \r { \t \n \r \"a\" \t \n \r : \t \n \r 2 \t \n \r } \t \n \r ] \t \n \r ";
        rjson_value* val = rjson_parse(json);
        assert_true(val != NULL, "Should handle excessive whitespace");
        rjson_free(val);
    }

    // TEST 30: Missing Colon / Value
    {
        printf("\n--- Test: Missing Colon / Value ---\n");
        const char* invalid[] = { "{\"a\":}", "{\"a\"}", "{\"a\" 1}", NULL };
        for (int i = 0; invalid[i]; i++) {
            rjson_value* val = rjson_parse(invalid[i]);
            assert_false(val != NULL, "Should reject missing colon/value");
            if (val) rjson_free(val);
        }
    }

    // TEST 31: Large String (1MB)
    // Tests memory allocation and stability with large inputs.
    {
        printf("\n--- Test: Large String (1MB) ---\n");
        size_t size = 1024 * 1024;
        char* large_json = (char*)malloc(size + 10);
        if (large_json) {
            large_json[0] = '"';
            memset(large_json + 1, 'a', size);
            large_json[size + 1] = '"';
            large_json[size + 2] = '\0';
            
            rjson_value* val = rjson_parse(large_json);
            assert_true(val != NULL, "Should parse 1MB string");
            if (val) {
                assert_true(strlen(val->as.str_val) == size, "String length should match");
                rjson_free(val);
            }
            free(large_json);
        }
    }

    printf("\n=== Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return (tests_failed == 0) ? 0 : 1;
}