#include "object.h"
#include <string.h>

static rjson_value* alloc_value(rjson_zone* zone, rjson_type type) {
    rjson_value* val = (rjson_value*)rjson_zone_alloc(zone, sizeof(rjson_value));
    if (val) {
        val->type = type;
        memset(&val->as, 0, sizeof(val->as)); // Clear union
    }
    return val;
}

rjson_value* rjson_object_new(rjson_zone* zone) {
    return alloc_value(zone, RJSON_OBJECT);
}

rjson_value* rjson_array_new(rjson_zone* zone) {
    return alloc_value(zone, RJSON_ARRAY);
}

rjson_value* rjson_string_new(rjson_zone* zone, const char* s, size_t len) {
    rjson_value* val = alloc_value(zone, RJSON_STRING);
    if (val) {
        // String contents are typically already in the zone or are in-situ.
        // We just store the pointer and length.
        val->as.str_val.ptr = s;
        val->as.str_val.len = len;
    }
    return val;
}

rjson_value* rjson_number_new(rjson_zone* zone, double n) {
    rjson_value* val = alloc_value(zone, RJSON_NUMBER);
    if (val) val->as.num_val = n;
    return val;
}

rjson_value* rjson_bool_new(rjson_zone* zone, int b) {
    rjson_value* val = alloc_value(zone, RJSON_BOOL);
    if (val) val->as.bool_val = b ? 1 : 0;
    return val;
}

rjson_value* rjson_null_new(rjson_zone* zone) {
    return alloc_value(zone, RJSON_NULL);
}

int rjson_object_add(rjson_zone* zone, rjson_value* object, const char* key, size_t key_len, rjson_value* value) {
    if (!object || object->type != RJSON_OBJECT || !key || !value) return -1;
    
    rjson_object* obj = &object->as.obj_val;
    if (obj->count >= obj->capacity) {
        size_t new_cap = obj->capacity == 0 ? 4 : obj->capacity * 2;
        rjson_kv* new_kvs = (rjson_kv*)rjson_zone_alloc(zone, new_cap * sizeof(rjson_kv));
        if (!new_kvs) return -1;
        if (obj->count > 0) {
            memcpy(new_kvs, obj->kvs, obj->count * sizeof(rjson_kv));
        }
        obj->kvs = new_kvs;
        obj->capacity = new_cap;
    }
    
    rjson_kv* kv = &obj->kvs[obj->count++];
    kv->key.ptr = key;
    kv->key.len = key_len;
    kv->value = *value;
    return 0;
}

int rjson_array_add(rjson_zone* zone, rjson_value* array, rjson_value* value) {
    if (!array || array->type != RJSON_ARRAY || !value) return -1;
    
    rjson_array* arr = &array->as.arr_val;
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 4 : arr->capacity * 2;
        rjson_value* new_els = (rjson_value*)rjson_zone_alloc(zone, new_cap * sizeof(rjson_value));
        if (!new_els) return -1;
        if (arr->count > 0) {
            memcpy(new_els, arr->elements, arr->count * sizeof(rjson_value));
        }
        arr->elements = new_els;
        arr->capacity = new_cap;
    }
    
    arr->elements[arr->count++] = *value;
    return 0;
}

rjson_value* rjson_object_get(const rjson_value* object, const char* key, size_t key_len) {
    if (!object || object->type != RJSON_OBJECT || !key) return NULL;
    
    const rjson_object* obj = &object->as.obj_val;
    for (size_t i = 0; i < obj->count; ++i) {
        const rjson_kv* kv = &obj->kvs[i];
        if (kv->key.len == key_len && memcmp(kv->key.ptr, key, key_len) == 0) {
            // We must return a pointer to the value embedded in the kv struct
            return (rjson_value*)&kv->value;
        }
    }
    return NULL;
}
