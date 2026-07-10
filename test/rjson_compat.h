#ifndef RJSON_COMPAT_H
#define RJSON_COMPAT_H

#include "rjson.h"
#include <string.h>

// Compatibility wrappers for the test suite to use the old API signatures
// without having to rewrite the entire test suite.

static rjson_doc _last_test_doc;
static rjson_zone* _test_zone = NULL;

static inline rjson_value* rjson_parse_compat(const char* json) {
    _last_test_doc = rjson_parse(json);
    return _last_test_doc.root;
}

static inline rjson_value* rjson_parse_with_length_compat(const char* json, size_t length) {
    _last_test_doc = rjson_parse_with_length(json, length);
    return _last_test_doc.root;
}

static inline void rjson_free_compat(rjson_value* val) {
    rjson_free(&_last_test_doc);
    if (_test_zone) {
        rjson_zone_destroy(_test_zone);
        _test_zone = NULL;
    }
}

static inline void _init_test_zone() {
    if (!_test_zone) _test_zone = rjson_zone_create();
}

static inline rjson_value* rjson_object_new_compat() { _init_test_zone(); return rjson_object_new(_test_zone); }
static inline rjson_value* rjson_array_new_compat() { _init_test_zone(); return rjson_array_new(_test_zone); }
static inline rjson_value* rjson_number_new_compat(double n) { _init_test_zone(); return rjson_number_new(_test_zone, n); }
static inline rjson_value* rjson_bool_new_compat(int b) { _init_test_zone(); return rjson_bool_new(_test_zone, b); }
static inline rjson_value* rjson_null_new_compat() { _init_test_zone(); return rjson_null_new(_test_zone); }
static inline rjson_value* rjson_string_new_compat(const char* s) { _init_test_zone(); return rjson_string_new(_test_zone, s, strlen(s)); }

static inline int rjson_object_add_compat(rjson_value* obj, const char* key, rjson_value* val) {
    _init_test_zone();
    return rjson_object_add(_test_zone, obj, key, strlen(key), val);
}
static inline int rjson_array_add_compat(rjson_value* arr, rjson_value* val) {
    _init_test_zone();
    return rjson_array_add(_test_zone, arr, val);
}

// Redefine old API to use compat wrappers
#define rjson_parse rjson_parse_compat
#define rjson_parse_with_length rjson_parse_with_length_compat
#define rjson_free rjson_free_compat
#define rjson_object_new rjson_object_new_compat
#define rjson_array_new rjson_array_new_compat
#define rjson_number_new rjson_number_new_compat
#define rjson_bool_new rjson_bool_new_compat
#define rjson_null_new rjson_null_new_compat
#define rjson_string_new rjson_string_new_compat
#define rjson_object_add rjson_object_add_compat
#define rjson_array_add rjson_array_add_compat

#endif
