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

void rjson_object_init(rjson_value* val);
void rjson_array_init(rjson_value* val);
void rjson_string_init(rjson_value* val, const char* s, size_t len);
void rjson_number_init(rjson_value* val, double n);
void rjson_bool_init(rjson_value* val, int b);
void rjson_null_init(rjson_value* val);

// --- Access ---
rjson_value* rjson_object_get(const rjson_value* object, const char* key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif // RJSON_OBJECT_H
