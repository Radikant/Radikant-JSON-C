#include "rjson.h"
#include <string.h>

void rjson_object_init(rjson_value* val) {
    if (!val) return;
    rjson_set_tag(val, RJSON_OBJECT, 0);
    val->as.kvs = NULL;
}

void rjson_array_init(rjson_value* val) {
    if (!val) return;
    rjson_set_tag(val, RJSON_ARRAY, 0);
    val->as.elements = NULL;
}

void rjson_string_init(rjson_value* val, const char* s, size_t len) {
    if (!val) return;
    rjson_set_tag(val, RJSON_STRING, len);
    val->as.str_val = s;
}

void rjson_number_init(rjson_value* val, double n) {
    if (!val) return;
    rjson_set_tag(val, RJSON_NUMBER, 0);
    val->as.num_val = n;
}

void rjson_bool_init(rjson_value* val, int b) {
    if (!val) return;
    rjson_set_tag(val, RJSON_BOOL, 0);
    val->as.bool_val = b ? 1 : 0;
}

void rjson_null_init(rjson_value* val) {
    if (!val) return;
    rjson_set_tag(val, RJSON_NULL, 0);
    memset(&val->as, 0, sizeof(val->as));
}

static rjson_value* alloc_value(rjson_zone* zone) {
    rjson_value* val = (rjson_value*)rjson_zone_alloc(zone, sizeof(rjson_value));
    if (val) memset(&val->as, 0, sizeof(val->as));
    return val;
}

rjson_value* rjson_object_new(rjson_zone* zone) {
    rjson_value* val = alloc_value(zone);
    rjson_object_init(val);
    return val;
}

rjson_value* rjson_array_new(rjson_zone* zone) {
    rjson_value* val = alloc_value(zone);
    rjson_array_init(val);
    return val;
}

rjson_value* rjson_string_new(rjson_zone* zone, const char* s, size_t len) {
    rjson_value* val = alloc_value(zone);
    rjson_string_init(val, s, len);
    return val;
}

rjson_value* rjson_number_new(rjson_zone* zone, double n) {
    rjson_value* val = alloc_value(zone);
    rjson_number_init(val, n);
    return val;
}

rjson_value* rjson_bool_new(rjson_zone* zone, int b) {
    rjson_value* val = alloc_value(zone);
    rjson_bool_init(val, b);
    return val;
}

rjson_value* rjson_null_new(rjson_zone* zone) {
    rjson_value* val = alloc_value(zone);
    rjson_null_init(val);
    return val;
}

rjson_value* rjson_object_get(const rjson_value* object, const char* key, size_t key_len) {
    if (!object || rjson_get_type(object) != RJSON_OBJECT || !key) return NULL;
    
    size_t count = rjson_get_len(object);
    const rjson_kv* kvs = object->as.kvs;
    for (size_t i = 0; i < count; ++i) {
        const rjson_value* k = &kvs[i].key;
        if (rjson_get_len(k) == key_len && memcmp(k->as.str_val, key, key_len) == 0) {
            return (rjson_value*)&kvs[i].value;
        }
    }
    return NULL;
}
