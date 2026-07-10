#include "perf.h"
#include "rjson.h"
#include "yyjson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// --- Real Radikant JSON Tests ---
static void parse_radikant(perf_result_t *res) {
    size_t size = 1024 * 512; // 500 KB
    char* large_json = malloc(size + 1);
    memset(large_json, ' ', size);
    large_json[0] = '[';
    for(int i=1; i<size-10; i+=10) {
        memcpy(large_json + i, "12345678, ", 10);
    }
    large_json[size-1] = ']';
    large_json[size] = '\0';
    
    int iterations = 20;
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        rjson_value *v = rjson_parse(large_json);
        if (v) rjson_free(v);
    }
    clock_t end = clock();
    
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
    double total_mb = (double)(size * iterations) / (1024.0 * 1024.0);
    
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001;
    
    perf_record_speed(res, total_mb / elapsed_sec);
    free(large_json);
}

static void serialize_radikant(perf_result_t *res) {
    rjson_value* root = rjson_array_new();
    for (int i=0; i<10000; i++) {
        rjson_array_add(root, rjson_number_new(i * 1.5));
    }
    
    int iterations = 20;
    double total_mb = 0;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        char* out_str = NULL;
        size_t out_len = 0;
        if (rjson_serialize(root, &out_str, &out_len) == 0 && out_str != NULL) {
            total_mb += (double)out_len / (1024.0 * 1024.0);
            free(out_str);
        }
    }
    clock_t end = clock();
    
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001;
    
    perf_record_speed(res, total_mb / elapsed_sec);
    rjson_free(root);
}


static void parse_yyjson(perf_result_t *res) {
    size_t size = 1024 * 512; // 500 KB
    char* large_json = malloc(size + 1);
    memset(large_json, ' ', size);
    large_json[0] = '[';
    for(int i=1; i<size-10; i+=10) {
        memcpy(large_json + i, "12345678, ", 10);
    }
    large_json[size-1] = ']';
    large_json[size] = '\0';
    
    int iterations = 20;
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        yyjson_doc *doc = yyjson_read(large_json, size, 0);
        if (doc) yyjson_doc_free(doc);
    }
    clock_t end = clock();
    
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
    double total_mb = (double)(size * iterations) / (1024.0 * 1024.0);
    
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001;
    
    perf_record_speed(res, total_mb / elapsed_sec);
    free(large_json);
}


static void serialize_yyjson(perf_result_t *res) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_doc_set_root(doc, arr);
    for (int i=0; i<10000; i++) {
        yyjson_mut_arr_add_real(doc, arr, i * 1.5);
    }
    
    int iterations = 20;
    double total_mb = 0;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        size_t out_len = 0;
        char* out_str = yyjson_mut_write(doc, 0, &out_len);
        if (out_str != NULL) {
            total_mb += (double)out_len / (1024.0 * 1024.0);
            free(out_str);
        }
    }
    clock_t end = clock();
    
    double elapsed_sec = (double)(end - start) / CLOCKS_PER_SEC;
    if (elapsed_sec <= 0.0) elapsed_sec = 0.001;
    
    perf_record_speed(res, total_mb / elapsed_sec);
    yyjson_mut_doc_free(doc);
}


int main()
{
    perf_suite_t suite;
    init_perf_suite(&suite, "Radikant JSON-C Performance");

    // PARSING
    add_perf_test(&suite, true,  true,  "PARSING", "radikant", parse_radikant);
    add_perf_test(&suite, false, true,  "PARSING", "yyjson", parse_yyjson);

    // SERIALIZATION
    add_perf_test(&suite, true,  true,  "SERIALIZATION", "radikant", serialize_radikant);
    add_perf_test(&suite, false, true,  "SERIALIZATION", "yyjson", serialize_yyjson);

    // Run tests & print tabular result
    run_perf_suite(&suite);

    // Cleanup
    free_perf_suite(&suite);

    return 0;
}