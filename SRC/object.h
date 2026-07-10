#ifndef RJSON_OBJECT_H
#define RJSON_OBJECT_H

#include "zone.h"
#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Node Creation ---
// Note: All nodes are allocated within the provided zone.
// They cannot be freed individually.

rjson_value* rjson_object_new(rjson_zone* zone);
rjson_value* rjson_array_new(rjson_zone* zone);
rjson_value* rjson_string_new(rjson_zone* zone, const char* s, size_t len);
rjson_value* rjson_number_new(rjson_zone* zone, double n);
rjson_value* rjson_bool_new(rjson_zone* zone, int b);
rjson_value* rjson_null_new(rjson_zone* zone);

// --- AST Modification ---
// Copies the provided value struct into the object/array's contiguous storage.
int rjson_object_add(rjson_zone* zone, rjson_value* object, const char* key, size_t key_len, rjson_value* value);
int rjson_array_add(rjson_zone* zone, rjson_value* array, rjson_value* value);

// --- Access ---
rjson_value* rjson_object_get(const rjson_value* object, const char* key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif // RJSON_OBJECT_H
