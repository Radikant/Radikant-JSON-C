#include "serialize.h"
#include <stdio.h>
#include <math.h>

#define RJSON_MAX_DEPTH 512

static rjson_error_t serialize_value(rjson_out_stream* stream, const rjson_value* value, int depth);

static rjson_error_t serialize_string(rjson_out_stream* stream, const rjson_string* str) {
    if (rjson_out_stream_append(stream, "\"", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    // Fast path: check if we need escaping
    int needs_escape = 0;
    for (size_t i = 0; i < str->len; i++) {
        unsigned char c = (unsigned char)str->ptr[i];
        if (c < 0x20 || c == '"' || c == '\\') {
            needs_escape = 1;
            break;
        }
    }
    
    if (!needs_escape) {
        if (rjson_out_stream_append(stream, str->ptr, str->len) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
        return rjson_out_stream_append(stream, "\"", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    }
    
    // Slow path: escape characters
    for (size_t i = 0; i < str->len; i++) {
        unsigned char c = (unsigned char)str->ptr[i];
        switch (c) {
            case '"':  if (rjson_out_stream_append(stream, "\\\"", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\\': if (rjson_out_stream_append(stream, "\\\\", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\b': if (rjson_out_stream_append(stream, "\\b", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\f': if (rjson_out_stream_append(stream, "\\f", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\n': if (rjson_out_stream_append(stream, "\\n", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\r': if (rjson_out_stream_append(stream, "\\r", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            case '\t': if (rjson_out_stream_append(stream, "\\t", 2) != 0) return RJSON_ERROR_SERIALIZE_NOMEM; break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    if (rjson_out_stream_append(stream, buf, 6) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
                } else {
                    if (rjson_out_stream_append(stream, (const char*)&c, 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
                }
                break;
        }
    }
    return rjson_out_stream_append(stream, "\"", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}

static rjson_error_t serialize_array(rjson_out_stream* stream, const rjson_array* arr, int depth) {
    if (depth >= RJSON_MAX_DEPTH) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    if (rjson_out_stream_append(stream, "[", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    for (size_t i = 0; i < arr->count; i++) {
        rjson_error_t err = serialize_value(stream, &arr->elements[i], depth + 1);
        if (err != RJSON_OK) return err;
        
        if (i < arr->count - 1) {
            if (rjson_out_stream_append(stream, ",", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
        }
    }
    
    return rjson_out_stream_append(stream, "]", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}

static rjson_error_t serialize_object(rjson_out_stream* stream, const rjson_object* obj, int depth) {
    if (depth >= RJSON_MAX_DEPTH) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    if (rjson_out_stream_append(stream, "{", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    for (size_t i = 0; i < obj->count; i++) {
        rjson_error_t err = serialize_string(stream, &obj->kvs[i].key);
        if (err != RJSON_OK) return err;
        
        if (rjson_out_stream_append(stream, ":", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
        
        err = serialize_value(stream, &obj->kvs[i].value, depth + 1);
        if (err != RJSON_OK) return err;
        
        if (i < obj->count - 1) {
            if (rjson_out_stream_append(stream, ",", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
        }
    }
    
    return rjson_out_stream_append(stream, "}", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}

static rjson_error_t serialize_value(rjson_out_stream* stream, const rjson_value* value, int depth) {
    if (!value) return RJSON_ERROR_OBJECT_NULL_POINTER;
    
    switch (value->type) {
        case RJSON_NULL:
            return rjson_out_stream_append(stream, "null", 4) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
        case RJSON_BOOL:
            if (value->as.bool_val) {
                return rjson_out_stream_append(stream, "true", 4) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
            } else {
                return rjson_out_stream_append(stream, "false", 5) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
            }
        case RJSON_NUMBER: {
            char buf[64];
            double n = value->as.num_val;
            if (floor(n) == n && n >= -9007199254740992.0 && n <= 9007199254740992.0) {
                // Integer
                int len = snprintf(buf, sizeof(buf), "%.0f", n);
                return rjson_out_stream_append(stream, buf, len) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
            } else {
                int len = snprintf(buf, sizeof(buf), "%.17g", n);
                return rjson_out_stream_append(stream, buf, len) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
            }
        }
        case RJSON_STRING:
            return serialize_string(stream, &value->as.str_val);
        case RJSON_ARRAY:
            return serialize_array(stream, &value->as.arr_val, depth);
        case RJSON_OBJECT:
            return serialize_object(stream, &value->as.obj_val, depth);
        default:
            return RJSON_ERROR_OBJECT_TYPE_MISMATCH;
    }
}

rjson_error_t rjson_serialize_stream(rjson_out_stream* stream, const rjson_value* value) {
    if (!stream || !value) return RJSON_ERROR_BAD_ARG;
    return serialize_value(stream, value, 0);
}
