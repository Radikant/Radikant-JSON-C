#ifndef RJSON_TYPES_H
#define RJSON_TYPES_H

#include <stddef.h>
#include <stdint.h>

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
typedef struct rjson_kv rjson_kv;

struct rjson_value {
    uint64_t tag; // 3 bits type, 61 bits length/count
    union {
        int bool_val;
        double num_val;
        const char* str_val;
        rjson_value* elements;
        rjson_kv* kvs;
    } as;
};

struct rjson_kv {
    rjson_value key;
    rjson_value value;
};

#ifdef __cplusplus
}
#endif

#endif // RJSON_TYPES_H
