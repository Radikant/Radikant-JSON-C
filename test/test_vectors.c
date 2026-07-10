#include "probe.h"
#include "rjson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>

test_suite_t vector_suite = {
    .name = "Radikant Vector Test Suite",
    .standard = "RFC-8259"
};

typedef enum {
    EXPECT_PASS,
    EXPECT_FAIL,
    EXPECT_NOCRASH
} test_mode_t;

static void test_directory(test_result_t *test, const char *dir_path, test_mode_t mode) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        char err[1024];
        snprintf(err, sizeof(err), "Failed to open directory: %s", dir_path);
        append_error(test, err, 0);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".json") != NULL) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
            
            FILE *f = fopen(path, "rb");
            if (!f) continue;
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *content = malloc(fsize + 1);
            if (fread(content, 1, fsize, f) != (size_t)fsize && fsize > 0) {
                // handle read error? Just ignore for now
            }
            fclose(f);
            content[fsize] = 0;
            
            rjson_value *v = rjson_parse_with_length(content, fsize);
            if (mode == EXPECT_PASS && v == NULL) {
                char err[1024];
                snprintf(err, sizeof(err), "Expected parse success for %s", path);
                append_error(test, err, 0);
                printf("FAIL (Expected Pass, got NULL): %s\n", path);
            } else if (mode == EXPECT_FAIL && v != NULL) {
                char err[1024];
                snprintf(err, sizeof(err), "Expected parse failure for %s", path);
                append_error(test, err, 0);
                printf("FAIL (Expected Fail, got Parse Success): %s\n", path);
            }
            
            if (v) rjson_free(v);
            free(content);
        }
    }
    closedir(dir);
}

bool test_normal(test_result_t *test) {
    test_directory(test, "../../test/vectors/test/normal", EXPECT_PASS);
    return test_end(test);
}

bool test_edge(test_result_t *test) {
    test_directory(test, "../../test/vectors/test/edge", EXPECT_PASS);
    return test_end(test);
}

bool test_mallicious(test_result_t *test) {
    test_directory(test, "../../test/vectors/attack/mallicious", EXPECT_NOCRASH);
    return test_end(test);
}

bool test_malformed(test_result_t *test) {
    test_directory(test, "../../test/vectors/attack/malformed", EXPECT_FAIL);
    return test_end(test);
}

bool test_nst_valid(test_result_t *test) {
    test_directory(test, "../../test/vectors/nst/nst_valid", EXPECT_PASS);
    return test_end(test);
}

bool test_nst_invalid(test_result_t *test) {
    test_directory(test, "../../test/vectors/nst/nst_invalid", EXPECT_FAIL);
    return test_end(test);
}

bool test_nst_impl(test_result_t *test) {
    test_directory(test, "../../test/vectors/nst/nst_impl", EXPECT_NOCRASH);
    return test_end(test);
}

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&vector_suite);

    add_test(&vector_suite, test_normal, "Normal Vectors", "RFC-8259");
    add_test(&vector_suite, test_edge, "Edge Cases", "RFC-8259");
    add_test(&vector_suite, test_mallicious, "Mallicious Vectors", "RFC-8259");
    add_test(&vector_suite, test_malformed, "Malformed Vectors", "RFC-8259");
    add_test(&vector_suite, test_nst_valid, "NST Valid", "RFC-8259");
    add_test(&vector_suite, test_nst_invalid, "NST Invalid", "RFC-8259");
    add_test(&vector_suite, test_nst_impl, "NST Implementation Defined", "RFC-8259");

    register_suite(&vector_suite);
    bool success = run_all_suites();

    free_suite(&vector_suite);
    return success ? 0 : 1;
}
