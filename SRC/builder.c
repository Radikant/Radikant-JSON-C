#include "builder.h"
#include <stdio.h>
#include <math.h>

rjson_error_t rjson_builder_init(rjson_builder* builder, rjson_out_stream* stream) {
    if (!builder || !stream) return RJSON_ERROR_BAD_ARG;
    builder->stream = stream;
    builder->depth = 0;
    builder->expects_value[0] = 0;
    builder->needs_comma[0] = 0;
    return RJSON_OK;
}

static rjson_error_t check_prefix(rjson_builder* b, int is_key) {
    if (b->depth < 0 || b->depth >= RJSON_BUILDER_MAX_DEPTH) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    
    int needs_comma = b->needs_comma[b->depth];
    if (needs_comma && !b->expects_value[b->depth]) {
        if (rjson_out_stream_append(b->stream, ",", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    }
    
    if (b->expects_value[b->depth] && is_key) {
        return RJSON_ERROR_OBJECT_TYPE_MISMATCH; // Expected value, got key
    }
    
    if (is_key) {
        b->expects_value[b->depth] = 1;
        b->needs_comma[b->depth] = 0;
    } else {
        if (b->expects_value[b->depth]) {
            if (rjson_out_stream_append(b->stream, ":", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
            b->expects_value[b->depth] = 0;
        }
        b->needs_comma[b->depth] = 1;
    }
    return RJSON_OK;
}

rjson_error_t rjson_builder_start_object(rjson_builder* builder) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    
    if (rjson_out_stream_append(builder->stream, "{", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    builder->depth++;
    if (builder->depth >= RJSON_BUILDER_MAX_DEPTH) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    
    builder->expects_value[builder->depth] = 0;
    builder->needs_comma[builder->depth] = 0;
    return RJSON_OK;
}

rjson_error_t rjson_builder_end_object(rjson_builder* builder) {
    if (builder->depth <= 0) return RJSON_ERROR_OBJECT_OUT_OF_BOUNDS;
    builder->depth--;
    return rjson_out_stream_append(builder->stream, "}", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}

rjson_error_t rjson_builder_start_array(rjson_builder* builder) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    
    if (rjson_out_stream_append(builder->stream, "[", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    builder->depth++;
    if (builder->depth >= RJSON_BUILDER_MAX_DEPTH) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    
    builder->expects_value[builder->depth] = 0;
    builder->needs_comma[builder->depth] = 0;
    return RJSON_OK;
}

rjson_error_t rjson_builder_end_array(rjson_builder* builder) {
    if (builder->depth <= 0) return RJSON_ERROR_OBJECT_OUT_OF_BOUNDS;
    builder->depth--;
    return rjson_out_stream_append(builder->stream, "]", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}

static rjson_error_t write_escaped_string(rjson_out_stream* stream, const char* str, size_t len) {
    if (rjson_out_stream_append(stream, "\"", 1) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    
    int needs_escape = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x20 || c == '"' || c == '\\') {
            needs_escape = 1;
            break;
        }
    }
    
    if (!needs_escape) {
        if (rjson_out_stream_append(stream, str, len) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
        return rjson_out_stream_append(stream, "\"", 1) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    }
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
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

rjson_error_t rjson_builder_write_key(rjson_builder* builder, const char* key, size_t len) {
    rjson_error_t err = check_prefix(builder, 1);
    if (err != RJSON_OK) return err;
    return write_escaped_string(builder->stream, key, len);
}

rjson_error_t rjson_builder_write_string(rjson_builder* builder, const char* str, size_t len) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    return write_escaped_string(builder->stream, str, len);
}

rjson_error_t rjson_builder_write_number(rjson_builder* builder, double num) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    
    char buf[64];
    if (floor(num) == num && num >= -9007199254740992.0 && num <= 9007199254740992.0) {
        int len = snprintf(buf, sizeof(buf), "%.0f", num);
        return rjson_out_stream_append(builder->stream, buf, len) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    } else {
        int len = snprintf(buf, sizeof(buf), "%.17g", num);
        return rjson_out_stream_append(builder->stream, buf, len) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    }
}

rjson_error_t rjson_builder_write_bool(rjson_builder* builder, int val) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    if (val) {
        return rjson_out_stream_append(builder->stream, "true", 4) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    } else {
        return rjson_out_stream_append(builder->stream, "false", 5) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
    }
}

rjson_error_t rjson_builder_write_null(rjson_builder* builder) {
    rjson_error_t err = check_prefix(builder, 0);
    if (err != RJSON_OK) return err;
    return rjson_out_stream_append(builder->stream, "null", 4) == 0 ? RJSON_OK : RJSON_ERROR_SERIALIZE_NOMEM;
}
