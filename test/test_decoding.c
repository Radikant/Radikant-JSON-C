#include "probe.h"
#include "rjson_compat.h"
#include "rjson_compat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Radikant
#include <radikant-json.h>
#include "rjson_compat.h"
#include <radikant-probe-c.h>

bool decode_test_1(test_result_t *test);
bool decode_test_2(test_result_t *test);

bool decode_edge_1(test_result_t *test);
bool decode_edge_2(test_result_t *test);
bool decode_edge_3(test_result_t *test);
bool decode_edge_4(test_result_t *test);
bool decode_edge_5(test_result_t *test);
bool decode_edge_6(test_result_t *test);
bool decode_edge_7(test_result_t *test);
bool decode_edge_8(test_result_t *test);
bool decode_edge_9(test_result_t *test);
bool decode_edge_10(test_result_t *test);
bool decode_edge_11(test_result_t *test);
bool decode_edge_12(test_result_t *test);
bool decode_edge_13(test_result_t *test);
bool decode_edge_14(test_result_t *test);
bool decode_edge_15(test_result_t *test);
bool decode_edge_16(test_result_t *test);
bool decode_edge_17(test_result_t *test);
bool decode_edge_18(test_result_t *test);
bool decode_edge_19(test_result_t *test);
bool decode_edge_20(test_result_t *test);
bool decode_edge_21(test_result_t *test);
bool decode_edge_22(test_result_t *test);
bool decode_edge_23(test_result_t *test);
bool decode_edge_24(test_result_t *test);
bool decode_edge_25(test_result_t *test);
bool decode_edge_26(test_result_t *test);
bool decode_edge_27(test_result_t *test);
bool decode_edge_28(test_result_t *test);
bool decode_edge_29(test_result_t *test);
bool decode_edge_30(test_result_t *test);
bool decode_edge_31(test_result_t *test);

test_suite_t decode_suite = {
    .name = "Radikant JSON-C Decoding Suite",
    .standard = "RFC-8259"
};

const char* sample_json = "{\n"
    "  \"name\": \"Radikant-JSON-C\",\n"
    "  \"version\": 1.0,\n"
    "  \"is_beta\": false,\n"
    "  \"author\": null,\n"
    "  \"features\": [\n"
    "    \"Parsing\",\n"
    "    \"Tree structure\",\n"
    "    \"Memory management\"\n"
    "  ],\n"
    "  \"details\": {\n"
    "    \"language\": \"C\",\n"
    "    \"lines_of_code\": 300\n"
    "  }\n"
    "}";


int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&decode_suite);

    add_test(&decode_suite, decode_test_1, "Decode valid JSON", "RFC-8259");
    add_test(&decode_suite, decode_test_2, "Verify parsed values", "RFC-8259");

    add_test(&decode_suite, decode_edge_1, "Stack Exhaustion Protection", "RFC-8259");
    add_test(&decode_suite, decode_edge_2, "Unicode Surrogate Pair Decoding", "RFC-8259");
    add_test(&decode_suite, decode_edge_3, "BOM (Byte Order Mark) Handling", "RFC-8259");
    add_test(&decode_suite, decode_edge_4, "Strict Number Parsing (Leading Zeros)", "RFC-8259");
    add_test(&decode_suite, decode_edge_5, "Unescaped Control Characters", "RFC-8259");
    add_test(&decode_suite, decode_edge_6, "Trailing Commas", "RFC-8259");
    add_test(&decode_suite, decode_edge_7, "Invalid Number Formats", "RFC-8259");
    add_test(&decode_suite, decode_edge_8, "Garbage after valid JSON", "RFC-8259");
    add_test(&decode_suite, decode_edge_9, "Invalid Unicode Escapes", "RFC-8259");
    add_test(&decode_suite, decode_edge_10, "Comments (Should Fail)", "RFC-8259");
    add_test(&decode_suite, decode_edge_11, "Top-level Scalars", "RFC-8259");
    add_test(&decode_suite, decode_edge_12, "Number Overflow", "RFC-8259");
    add_test(&decode_suite, decode_edge_13, "Invalid Escapes (Extended)", "RFC-8259");
    add_test(&decode_suite, decode_edge_14, "Case Sensitivity", "RFC-8259");
    add_test(&decode_suite, decode_edge_15, "Deeply Nested Objects", "RFC-8259");
    add_test(&decode_suite, decode_edge_16, "Lone Surrogates (Invalid UTF-8)", "RFC-8259");
    add_test(&decode_suite, decode_edge_17, "Null Bytes in Strings", "RFC-8259");
    add_test(&decode_suite, decode_edge_18, "Duplicate Keys", "RFC-8259");
    add_test(&decode_suite, decode_edge_19, "Keywords as Keys", "RFC-8259");
    add_test(&decode_suite, decode_edge_20, "Strict Whitespace", "RFC-8259");
    add_test(&decode_suite, decode_edge_21, "Invalid Array Structure", "RFC-8259");
    add_test(&decode_suite, decode_edge_22, "Invalid Object Structure", "RFC-8259");
    add_test(&decode_suite, decode_edge_23, "Mismatched Brackets", "RFC-8259");
    add_test(&decode_suite, decode_edge_24, "Incomplete JSON (EOF)", "RFC-8259");
    add_test(&decode_suite, decode_edge_25, "Tricky Valid Numbers", "RFC-8259");
    add_test(&decode_suite, decode_edge_26, "Escaped Forward Slash", "RFC-8259");
    add_test(&decode_suite, decode_edge_27, "Raw UTF-8 Input", "RFC-8259");
    add_test(&decode_suite, decode_edge_28, "Empty Structures", "RFC-8259");
    add_test(&decode_suite, decode_edge_29, "Whitespace Torture", "RFC-8259");
    add_test(&decode_suite, decode_edge_30, "Missing Colon / Value", "RFC-8259");
    add_test(&decode_suite, decode_edge_31, "Large String (1MB)", "RFC-8259");

    register_suite(&decode_suite);
    bool success = run_all_suites();

    free_suite(&decode_suite);
    return success ? 0 : 1;
}

bool decode_test_1(test_result_t *test) {
    rjson_value* parsed_json = rjson_parse(sample_json);
    if (parsed_json == NULL) {
        append_error(test, "Failed to parse valid JSON", 0);
    } else {
        rjson_free(parsed_json);
    }
    return test_end(test);
}

bool decode_test_2(test_result_t *test) {
    rjson_value* parsed_json = rjson_parse(sample_json);
    if (parsed_json == NULL) {
        append_error(test, "Failed to parse valid JSON", 0);
        return test_end(test);
    }

    rjson_value* name_val = rjson_object_get_value(parsed_json, "name");
    if (!name_val || name_val->type != RJSON_STRING || name_val->as.str_val.len != 15 || strncmp(name_val->as.str_val.ptr, "Radikant-JSON-C", 15) != 0) {
        append_error(test, "Incorrect value for 'name'", 0);
    }

    rjson_value* version_val = rjson_object_get_value(parsed_json, "version");
    if (!version_val || version_val->type != RJSON_NUMBER || version_val->as.num_val != 1.0) {
        append_error(test, "Incorrect value for 'version'", 0);
    }

    rjson_free(parsed_json);
    return test_end(test);
}

bool decode_edge_1(test_result_t *test) {
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
            if (val != NULL) { append_error(test, "Deeply nested array (600) should fail gracefully", 0); }
            if (val)
                rjson_free(val);
            free(deep_json);
        }
    
    return test_end(test);
}

bool decode_edge_2(test_result_t *test) {
        const char *json = "\"\\uD83D\\uDE00\"";
        rjson_value *val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should parse surrogate pair escape sequence", 0); }

        if (val && val->type == RJSON_STRING)
        {
            unsigned char *bytes = (unsigned char *)val->as.str_val.ptr;
            // Check for UTF-8 encoding of U+1F600
            int is_correct = (bytes[0] == 0xF0 && bytes[1] == 0x9F &&
                              bytes[2] == 0x98 && bytes[3] == 0x80);
            if (!(is_correct)) { append_error(test, "Should decode to correct UTF-8 bytes (😀)", 0); }
        }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_3(test_result_t *test) {
        const char *json = "\xEF\xBB\xBF{\"a\":1}";
        rjson_value *val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should ignore UTF-8 BOM at start of file", 0); }
        if (val) rjson_free(val);
    return test_end(test);
}

bool decode_edge_4(test_result_t *test) {
        const char *json = "01";
        rjson_value *val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject numbers with leading zeros (e.g., 01)", 0); }
        if (val)
            rjson_free(val);

        json = "0";
        val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept single zero", 0); }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_5(test_result_t *test) {
        const char *json = "\"Line\nBreak\"";
        rjson_value *val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject unescaped newline in string", 0); }
        if (val)
            rjson_free(val);

        json = "\"Line\\nBreak\""; // Valid escaped
        val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept escaped newline", 0); }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_6(test_result_t *test) {
        const char* json = "[1, 2, 3,]";
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject trailing comma in array", 0); }
        if (val) rjson_free(val);

        json = "{\"a\": 1,}";
        val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject trailing comma in object", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_7(test_result_t *test) {
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
                if (1) { append_error(test, "Should reject invalid number format", 0); }
            } else {
                if (!(1)) { append_error(test, "Rejected invalid number format", 0); }
            }
        }
    
    return test_end(test);
}

bool decode_edge_8(test_result_t *test) {
        const char* json = "{} garbage";
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject content after valid JSON", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_9(test_result_t *test) {
        const char* json = "\"\\u123\""; // Too short
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject short \\u sequence", 0); }
        if (val) rjson_free(val);

        json = "\"\\u12GG\""; // Invalid hex
        val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject invalid hex in \\u sequence", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_10(test_result_t *test) {
        const char* json = "[1, 2 /* comment */]";
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject inline comments", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_11(test_result_t *test) {
        rjson_value* val = rjson_parse("\"hello\"");
        if (!(val != NULL && val->type == RJSON_STRING)) { append_error(test, "Should parse top-level string", 0); }
        rjson_free(val);

        val = rjson_parse("123");
        if (!(val != NULL && val->type == RJSON_NUMBER)) { append_error(test, "Should parse top-level number", 0); }
        rjson_free(val);

        val = rjson_parse("true");
        if (!(val != NULL && val->type == RJSON_BOOL)) { append_error(test, "Should parse top-level boolean", 0); }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_12(test_result_t *test) {
        const char* json = "1e309";
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject number overflow (Infinity)", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_13(test_result_t *test) {
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
                if (1) { append_error(test, "Should reject invalid escape", 0); }
            } else {
                if (!(1)) { append_error(test, "Rejected invalid escape", 0); }
            }
        }
    
    return test_end(test);
}

bool decode_edge_14(test_result_t *test) {
        const char* invalid_literals[] = { "True", "FALSE", "Null", "NULL", NULL };
        for (int i = 0; invalid_literals[i]; i++) {
            rjson_value* val = rjson_parse(invalid_literals[i]);
            if (val != NULL) { append_error(test, "Should reject incorrect case literal", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_15(test_result_t *test) {
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
            if (val != NULL) { append_error(test, "Deeply nested object (600) should fail gracefully", 0); }
            if (val) rjson_free(val);
            free(deep_json);
        }
    
    return test_end(test);
}

bool decode_edge_16(test_result_t *test) {
        const char* json = "\"\\uD800\"";
        rjson_value* val = rjson_parse(json);
        if (val != NULL) { append_error(test, "Should reject lone surrogate (invalid UTF-8)", 0); }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_17(test_result_t *test) {
        const char* json = "\"\\u0000\"";
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL && val->type == RJSON_STRING && val->as.str_val.len == 1 && val->as.str_val.ptr[0] == '\0')) { 
            append_error(test, "Should accept \\u0000 since strings have length now", 0); 
        }
        if (val) rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_18(test_result_t *test) {
        const char* json = "{\"a\": 1, \"a\": 2}";
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept duplicate keys (valid JSON)", 0); }
        
        // Optional: Check which value is preserved (usually last one wins or both kept)
        if (val) {
            rjson_value* inner = rjson_object_get_value(val, "a");
            if (!(inner != NULL)) { append_error(test, "Should be able to retrieve key 'a'", 0); }
            rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_19(test_result_t *test) {
        const char* json = "{\"true\": 1, \"null\": 2, \"false\": 3}";
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept keywords as object keys", 0); }
        if (val) {
            rjson_value* v = rjson_object_get_value(val, "true");
            if (!(v != NULL && v->type == RJSON_NUMBER)) { append_error(test, "Should retrieve 'true' key", 0); }
            rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_20(test_result_t *test) {
        const char* invalid_ws[] = { "[\f]", "[\v]", NULL };
        for (int i = 0; invalid_ws[i]; i++) {
            rjson_value* val = rjson_parse(invalid_ws[i]);
            if (val != NULL) { append_error(test, "Should reject non-JSON whitespace (\\f, \\v)", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_21(test_result_t *test) {
        const char* invalid_arrays[] = { "[1:2]", "[1 2]", NULL };
        for (int i = 0; invalid_arrays[i]; i++) {
            rjson_value* val = rjson_parse(invalid_arrays[i]);
            if (val != NULL) { append_error(test, "Should reject invalid array structure", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_22(test_result_t *test) {
        const char* invalid_objects[] = { "{\"a\", 1}", "{\"a\" 1}", NULL };
        for (int i = 0; invalid_objects[i]; i++) {
            rjson_value* val = rjson_parse(invalid_objects[i]);
            if (val != NULL) { append_error(test, "Should reject invalid object structure", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_23(test_result_t *test) {
        const char* invalid_brackets[] = { "[}", "{]", NULL };
        for (int i = 0; invalid_brackets[i]; i++) {
            rjson_value* val = rjson_parse(invalid_brackets[i]);
            if (val != NULL) { append_error(test, "Should reject mismatched brackets", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_24(test_result_t *test) {
    return test_end(test);
}

bool decode_edge_25(test_result_t *test) {
        const char* valid_nums[] = { "-0", "0e0", "0E+1", "0.0", "-0.0", NULL };
        for (int i = 0; valid_nums[i]; i++) {
            rjson_value* val = rjson_parse(valid_nums[i]);
            if (!(val != NULL && val->type == RJSON_NUMBER)) { append_error(test, "Should accept tricky valid number", 0); }
            rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_26(test_result_t *test) {
        const char* json = "\"\\/\"";
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept escaped forward slash", 0); }
        if (val && val->type == RJSON_STRING) {
            if (val->as.str_val.len != 1 || strncmp(val->as.str_val.ptr, "/", 1) != 0) { append_error(test, "Should decode \\/ to /", 0); }
        }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_27(test_result_t *test) {
        // "🔥" in UTF-8 is F0 9F 94 A5
        const char* json = "\"🔥\""; 
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should accept raw UTF-8 characters in string", 0); }
        if (val && val->type == RJSON_STRING) {
            // Check bytes
            unsigned char* bytes = (unsigned char*)val->as.str_val.ptr;
            int is_correct = (bytes[0] == 0xF0 && bytes[1] == 0x9F && 
                              bytes[2] == 0x94 && bytes[3] == 0xA5);
            if (!(is_correct)) { append_error(test, "Should preserve raw UTF-8 bytes", 0); }
        }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_28(test_result_t *test) {
        rjson_value* val = rjson_parse("[]");
        if (!(val != NULL && val->type == RJSON_ARRAY && val->as.arr_val.count == 0)) { append_error(test, "Should parse empty array", 0); }
        rjson_free(val);

        val = rjson_parse("{}");
        if (!(val != NULL && val->type == RJSON_OBJECT && val->as.obj_val.count == 0)) { append_error(test, "Should parse empty object", 0); }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_29(test_result_t *test) {
        const char* json = " \t \n \r [ \t \n \r 1 \t \n \r , \t \n \r { \t \n \r \"a\" \t \n \r : \t \n \r 2 \t \n \r } \t \n \r ] \t \n \r ";
        rjson_value* val = rjson_parse(json);
        if (!(val != NULL)) { append_error(test, "Should handle excessive whitespace", 0); }
        rjson_free(val);
    
    return test_end(test);
}

bool decode_edge_30(test_result_t *test) {
        const char* invalid[] = { "{\"a\":}", "{\"a\"}", "{\"a\" 1}", NULL };
        for (int i = 0; invalid[i]; i++) {
            rjson_value* val = rjson_parse(invalid[i]);
            if (val != NULL) { append_error(test, "Should reject missing colon/value", 0); }
            if (val) rjson_free(val);
        }
    
    return test_end(test);
}

bool decode_edge_31(test_result_t *test) {
        size_t size = 1024 * 1024;
        char* large_json = (char*)malloc(size + 10);
        if (large_json) {
            large_json[0] = '"';
            memset(large_json + 1, 'a', size);
            large_json[size + 1] = '"';
            large_json[size + 2] = '\0';
            
            rjson_value* val = rjson_parse(large_json);
            if (!(val != NULL)) { append_error(test, "Should parse 1MB string", 0); }
            if (val) {
                if (!(val->as.str_val.len == size)) { append_error(test, "String length should match", 0); }
                rjson_free(val);
            }
            free(large_json);
        }
    
    return test_end(test);
}