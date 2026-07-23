#include "perf.h"
#include "rjson_compat.h"
#include "yyjson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// Global Payloads
char* g_simple_json = NULL;
char* g_diff_json = NULL;
char* g_hard_json = NULL;
size_t g_simple_size = 0;
size_t g_diff_size = 0;
size_t g_hard_size = 0;

rjson_value* g_radikant_simple = NULL;
rjson_value* g_radikant_diff = NULL;
rjson_value* g_radikant_hard = NULL;

yyjson_mut_doc* g_yyjson_simple = NULL;
yyjson_mut_doc* g_yyjson_diff = NULL;
yyjson_mut_doc* g_yyjson_hard = NULL;

static void setup_payloads() {
    printf("Setting up simple...\n"); fflush(stdout);
    // Build Radikant trees
    g_radikant_simple = rjson_array_new();
    for (int i = 0; i < 10000; i++) rjson_array_add(g_radikant_simple, rjson_number_new(i * 1.5));
    
    printf("Setting up diff...\n"); fflush(stdout);
    g_radikant_diff = rjson_array_new();
    for (int i = 0; i < 1000; i++) {
        rjson_value* obj = rjson_object_new();
        rjson_object_add(obj, "id", rjson_number_new(i));
        rjson_object_add(obj, "name", rjson_string_new("User Name"));
        rjson_object_add(obj, "active", rjson_bool_new(i % 2 == 0));
        rjson_value* tags = rjson_array_new();
        rjson_array_add(tags, rjson_string_new("tag1"));
        rjson_array_add(tags, rjson_string_new("tag2"));
        rjson_object_add(obj, "tags", tags);
        rjson_array_add(g_radikant_diff, obj);
    }
    
    printf("Setting up hard...\n"); fflush(stdout);
    g_radikant_hard = rjson_array_new();
    for (int i = 0; i < 1000; i++) {
        rjson_value* obj = rjson_object_new();
        rjson_object_add(obj, "complex_float", rjson_number_new(1.23456789e-10 * i));
        rjson_object_add(obj, "escaped_string", rjson_string_new("Line1\nLine2\t\"Quotes\"\b\f"));
        
        rjson_value* deep = rjson_object_new();
        rjson_value* curr = deep;
        for (int j = 0; j < 20; j++) {
            rjson_value* next = rjson_object_new();
            rjson_object_add(curr, "nested", next);
            curr = next;
        }
        rjson_object_add(curr, "value", rjson_null_new());
        rjson_object_add(obj, "deep_nesting", deep);
        rjson_array_add(g_radikant_hard, obj);
    }

    printf("Encoding radikant simple...\n"); fflush(stdout);
    // Generate JSON strings
    rjson_encode(g_radikant_simple, &g_simple_json, &g_simple_size);
    rjson_encode(g_radikant_diff, &g_diff_json, &g_diff_size);
    rjson_encode(g_radikant_hard, &g_hard_json, &g_hard_size);
    
    printf("Parsing yyjson...\n"); fflush(stdout);
    // Parse strings into YYJSON mutable docs for encoding tests
    yyjson_doc* temp_simple = yyjson_read(g_simple_json, g_simple_size, 0);
    g_yyjson_simple = yyjson_doc_mut_copy(temp_simple, NULL);
    yyjson_doc_free(temp_simple);
    
    yyjson_doc* temp_diff = yyjson_read(g_diff_json, g_diff_size, 0);
    g_yyjson_diff = yyjson_doc_mut_copy(temp_diff, NULL);
    yyjson_doc_free(temp_diff);
    
    yyjson_doc* temp_hard = yyjson_read(g_hard_json, g_hard_size, 0);
    g_yyjson_hard = yyjson_doc_mut_copy(temp_hard, NULL);
    yyjson_doc_free(temp_hard);
}

static void teardown_payloads() {
    free(g_simple_json);
    free(g_diff_json);
    free(g_hard_json);
    // the radikant docs are managed by the compat layer zone, but we can just let it leak for the test program
    yyjson_mut_doc_free(g_yyjson_simple);
    yyjson_mut_doc_free(g_yyjson_diff);
    yyjson_mut_doc_free(g_yyjson_hard);
}

// Wrappers
static void* radikant_decode_wrapper(const char* str, size_t len) {
    return rjson_parse_with_length(str, len);
}
static void radikant_free_wrapper(void* p) {
    (void)p;
#undef rjson_free
    rjson_free(&_last_test_doc);
#define rjson_free rjson_free_compat
}
static void* yyjson_decode_wrapper(const char* str, size_t len) {
    return yyjson_read(str, len, 0);
}
static void yyjson_free_wrapper(void* p) {
    yyjson_doc_free((yyjson_doc*)p);
}
static char* radikant_encode_wrapper(void* tree, size_t* out_len) {
    char* str = NULL;
    rjson_encode((rjson_value*)tree, &str, out_len);
    return str;
}
static char* yyjson_encode_wrapper(void* tree, size_t* out_len) {
    return yyjson_mut_write((yyjson_mut_doc*)tree, 0, out_len);
}
static void std_free_wrapper(void* p) {
    free(p);
}

// Macros
#define DEFINE_DECODE_TEST(lib_name, payload, payload_size, decode_func, free_func) \
static void decode_##lib_name##_##payload(perf_result_t *res) { \
    int iterations = 100; \
    clock_t start = clock(); \
    for (int i = 0; i < iterations; i++) { \
        void* doc = decode_func(payload, payload_size); \
        if (doc) free_func(doc); \
    } \
    clock_t end = clock(); \
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC; \
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001; \
    double total_mb = (double)(payload_size * iterations) / (1024.0 * 1024.0); \
    perf_record_speed(res, total_mb / elapsed_sec); \
}

#define DEFINE_ENCODE_TEST(lib_name, payload, payload_tree, encode_func, free_str_func) \
static void encode_##lib_name##_##payload(perf_result_t *res) { \
    int iterations = 100; \
    double total_mb = 0; \
    clock_t start = clock(); \
    for (int i = 0; i < iterations; i++) { \
        size_t len = 0; \
        char* str = encode_func(payload_tree, &len); \
        if (str) { \
            total_mb += (double)len / (1024.0 * 1024.0); \
            free_str_func(str); \
        } \
    } \
    clock_t end = clock(); \
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC; \
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001; \
    perf_record_speed(res, total_mb / elapsed_sec); \
}

// Instantiate Tests
DEFINE_DECODE_TEST(radikant, g_simple_json, g_simple_size, radikant_decode_wrapper, radikant_free_wrapper)
DEFINE_DECODE_TEST(radikant, g_diff_json, g_diff_size, radikant_decode_wrapper, radikant_free_wrapper)
DEFINE_DECODE_TEST(radikant, g_hard_json, g_hard_size, radikant_decode_wrapper, radikant_free_wrapper)

DEFINE_DECODE_TEST(yyjson, g_simple_json, g_simple_size, yyjson_decode_wrapper, yyjson_free_wrapper)
DEFINE_DECODE_TEST(yyjson, g_diff_json, g_diff_size, yyjson_decode_wrapper, yyjson_free_wrapper)
DEFINE_DECODE_TEST(yyjson, g_hard_json, g_hard_size, yyjson_decode_wrapper, yyjson_free_wrapper)

DEFINE_ENCODE_TEST(radikant, g_simple_json, g_radikant_simple, radikant_encode_wrapper, std_free_wrapper)
DEFINE_ENCODE_TEST(radikant, g_diff_json, g_radikant_diff, radikant_encode_wrapper, std_free_wrapper)
DEFINE_ENCODE_TEST(radikant, g_hard_json, g_radikant_hard, radikant_encode_wrapper, std_free_wrapper)

DEFINE_ENCODE_TEST(yyjson, g_simple_json, g_yyjson_simple, yyjson_encode_wrapper, std_free_wrapper)
DEFINE_ENCODE_TEST(yyjson, g_diff_json, g_yyjson_diff, yyjson_encode_wrapper, std_free_wrapper)
DEFINE_ENCODE_TEST(yyjson, g_hard_json, g_yyjson_hard, yyjson_encode_wrapper, std_free_wrapper)

int main()
{
    setup_payloads();
    
    perf_suite_t suite;
    init_perf_suite(&suite, "Radikant JSON-C Performance");
    perf_suite_set_warmup(&suite, true, 100, false, NULL);

    add_perf_test(&suite, true,  true,  "Decoding (Simple)", "radikant", decode_radikant_g_simple_json);
    add_perf_test(&suite, false, true,  "Decoding (Simple)", "yyjson", decode_yyjson_g_simple_json);
    
    add_perf_test(&suite, true,  true,  "Decoding (Diff)", "radikant", decode_radikant_g_diff_json);
    add_perf_test(&suite, false, true,  "Decoding (Diff)", "yyjson", decode_yyjson_g_diff_json);
    
    add_perf_test(&suite, true,  true,  "Decoding (Hard)", "radikant", decode_radikant_g_hard_json);
    add_perf_test(&suite, false, true,  "Decoding (Hard)", "yyjson", decode_yyjson_g_hard_json);

    add_perf_test(&suite, true,  true,  "Encoding (Simple)", "radikant", encode_radikant_g_simple_json);
    add_perf_test(&suite, false, true,  "Encoding (Simple)", "yyjson", encode_yyjson_g_simple_json);
    
    add_perf_test(&suite, true,  true,  "Encoding (Diff)", "radikant", encode_radikant_g_diff_json);
    add_perf_test(&suite, false, true,  "Encoding (Diff)", "yyjson", encode_yyjson_g_diff_json);
    
    add_perf_test(&suite, true,  true,  "Encoding (Hard)", "radikant", encode_radikant_g_hard_json);
    add_perf_test(&suite, false, true,  "Encoding (Hard)", "yyjson", encode_yyjson_g_hard_json);

    run_perf_suite(&suite);
    free_perf_suite(&suite);
    
    teardown_payloads();

    return 0;
}