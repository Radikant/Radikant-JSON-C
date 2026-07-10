#ifndef RJSON_TYPES_H
#define RJSON_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RJSON_NULL,
    RJSON_BOOL,
    RJSON_NUMBER,
    RJSON_STRING,
    RJSON_ARRAY,
    RJSON_OBJECT
} rjson_type;

typedef struct rjson_value rjson_value;
typedef struct rjson_string rjson_string;
typedef struct rjson_array rjson_array;
typedef struct rjson_object rjson_object;
typedef struct rjson_kv rjson_kv;

struct rjson_string {
    const char* ptr;
    size_t len;
};

struct rjson_array {
    rjson_value* elements; // Contiguous block of rjson_value structs
    size_t count;
    size_t capacity;
};

struct rjson_object {
    rjson_kv* kvs;         // Contiguous block of key-value pairs
    size_t count;
    size_t capacity;
};

struct rjson_value {
    rjson_type type;
    union {
        int bool_val;
        double num_val;
        rjson_string str_val;
        rjson_array arr_val;
        rjson_object obj_val;
    } as;
};

// Must be defined AFTER rjson_value is fully defined!
struct rjson_kv {
    rjson_string key;
    rjson_value value;     // Embedded value struct
};

#ifdef __cplusplus
}
#endif

#endif // RJSON_TYPES_H
