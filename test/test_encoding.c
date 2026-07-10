#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Radikant
#include <radikant-json.h>
#include <radikant-probe-c.h>

bool encode_test_1(test_result_t *test);
bool encode_test_2(test_result_t *test);

test_suite_t encode_suite = {
    .name = "Radikant JSON-C Encoding Suite",
    .standard = "RFC-8259"
};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&encode_suite);

    add_test(&encode_suite, encode_test_1, "Encode basic object", "RFC-8259");
    add_test(&encode_suite, encode_test_2, "Encode nested structure", "RFC-8259");

    register_suite(&encode_suite);
    bool success = run_all_suites();

    free_suite(&encode_suite);
    return success ? 0 : 1;
}

bool encode_test_1(test_result_t *test) {
    rjson_value* root = rjson_object_new();
    rjson_object_add(root, "key", rjson_string_new("value"));

    char* out_str = NULL;
    size_t out_len = 0;
    
    if (rjson_serialize(root, &out_str, &out_len) != 0) {
        append_error(test, "Failed to serialize JSON", 0);
    } else if (out_str == NULL) {
        append_error(test, "Serialized string is NULL", 0);
    } else {
        printf("  [DEBUG] Serialized: %s\n", out_str);
        free(out_str);
    }
    
    rjson_free(root);
    return test_end(test);
}

bool encode_test_2(test_result_t *test) {
    rjson_value* root = rjson_object_new();
    rjson_value* arr = rjson_array_new();
    
    rjson_array_add(arr, rjson_number_new(1));
    rjson_array_add(arr, rjson_number_new(2));
    rjson_array_add(arr, rjson_number_new(3));
    
    rjson_object_add(root, "numbers", arr);
    rjson_object_add(root, "active", rjson_bool_new(1));
    
    char* out_str = NULL;
    
    if (rjson_serialize(root, &out_str, NULL) != 0) {
        append_error(test, "Failed to serialize nested JSON", 0);
    } else {
        printf("  [DEBUG] Serialized nested: %s\n", out_str);
        free(out_str);
    }

    rjson_free(root);
    return test_end(test);
}