#include "stream.h"
#include <stdlib.h>
#include <string.h>

int rjson_out_stream_init(rjson_out_stream* stream, size_t initial_capacity) {
    stream->buffer = (char*)malloc(initial_capacity);
    if (!stream->buffer) return -1;
    stream->length = 0;
    stream->capacity = initial_capacity;
    stream->buffer[0] = '\0';
    return 0;
}

int rjson_out_stream_grow(rjson_out_stream* stream, size_t min_needed) {
    size_t new_capacity = stream->capacity == 0 ? 64 : stream->capacity * 2;
    while (new_capacity < min_needed) {
        new_capacity *= 2;
    }
    char* new_buffer = (char*)realloc(stream->buffer, new_capacity);
    if (!new_buffer) return -1;
    stream->buffer = new_buffer;
    stream->capacity = new_capacity;
    return 0;
}

int rjson_out_stream_append(rjson_out_stream* stream, const char* data, size_t len) {
    if (stream->length + len + 1 > stream->capacity) {
        if (rjson_out_stream_grow(stream, stream->length + len + 1) != 0) return -1;
    }
    memcpy(stream->buffer + stream->length, data, len);
    stream->length += len;
    stream->buffer[stream->length] = '\0';
    return 0;
}

int rjson_out_stream_finish(rjson_out_stream* stream) {
    if (stream->length + 1 > stream->capacity) {
        if (rjson_out_stream_grow(stream, stream->length + 1) != 0) return -1;
    }
    stream->buffer[stream->length] = '\0';
    return 0;
}

void rjson_out_stream_destroy(rjson_out_stream* stream) {
    free(stream->buffer);
    stream->buffer = NULL;
}

int rjson_stream_push_val(rjson_stream* stream, rjson_value val) {
    if (stream->val_stack_count >= stream->val_stack_capacity) {
        size_t new_cap = stream->val_stack_capacity == 0 ? 64 : stream->val_stack_capacity * 2;
        rjson_value* new_stack = (rjson_value*)realloc(stream->val_stack, new_cap * sizeof(rjson_value));
        if (!new_stack) return -1;
        stream->val_stack = new_stack;
        stream->val_stack_capacity = new_cap;
    }
    stream->val_stack[stream->val_stack_count++] = val;
    return 0;
}

int rjson_stream_push_kv(rjson_stream* stream, rjson_kv kv) {
    if (stream->kv_stack_count >= stream->kv_stack_capacity) {
        size_t new_cap = stream->kv_stack_capacity == 0 ? 64 : stream->kv_stack_capacity * 2;
        rjson_kv* new_stack = (rjson_kv*)realloc(stream->kv_stack, new_cap * sizeof(rjson_kv));
        if (!new_stack) return -1;
        stream->kv_stack = new_stack;
        stream->kv_stack_capacity = new_cap;
    }
    stream->kv_stack[stream->kv_stack_count++] = kv;
    return 0;
}

void rjson_stream_destroy(rjson_stream* stream) {
    if (stream->val_stack) {
        free(stream->val_stack);
        stream->val_stack = NULL;
    }
    if (stream->kv_stack) {
        free(stream->kv_stack);
        stream->kv_stack = NULL;
    }
}
