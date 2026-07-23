#ifndef RJSON_STREAM_H
#define RJSON_STREAM_H

#include <stddef.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* data;
    size_t length;
    size_t position;
    
    rjson_value* val_stack;
    size_t val_stack_count;
    size_t val_stack_capacity;
    
    rjson_kv* kv_stack;
    size_t kv_stack_count;
    size_t kv_stack_capacity;
} rjson_stream;

static inline void rjson_stream_init(rjson_stream* stream, const char* data, size_t length) {
    stream->data = data;
    stream->length = length;
    stream->position = 0;
    stream->val_stack = NULL;
    stream->val_stack_count = 0;
    stream->val_stack_capacity = 0;
    stream->kv_stack = NULL;
    stream->kv_stack_count = 0;
    stream->kv_stack_capacity = 0;
}

static inline int rjson_stream_peek(rjson_stream* stream) {
    if (stream->position >= stream->length) return -1;
    return (unsigned char)stream->data[stream->position];
}

static inline int rjson_stream_get(rjson_stream* stream) {
    if (stream->position >= stream->length) return -1;
    return (unsigned char)stream->data[stream->position++];
}

static const unsigned char rjson_is_space[256] = {
    [' '] = 1, ['\n'] = 1, ['\r'] = 1, ['\t'] = 1
};

static inline void rjson_stream_skip_whitespace(rjson_stream* stream) {
    while (stream->position < stream->length) {
        if (rjson_is_space[(unsigned char)stream->data[stream->position]]) {
            stream->position++;
        } else {
            break;
        }
    }
}

static inline const char* rjson_stream_current_ptr(rjson_stream* stream) {
    return stream->data + stream->position;
}

static inline size_t rjson_stream_remaining(rjson_stream* stream) {
    return stream->length - stream->position;
}

// For serialization (output stream)
typedef struct {
    char* buffer;
    size_t length;
    size_t capacity;
} rjson_out_stream;

int rjson_out_stream_init(rjson_out_stream* stream, size_t initial_capacity);
int rjson_out_stream_append(rjson_out_stream* stream, const char* data, size_t len);
int rjson_out_stream_finish(rjson_out_stream* stream);
int rjson_out_stream_grow(rjson_out_stream* stream, size_t min_needed);
void rjson_out_stream_destroy(rjson_out_stream* stream);

// unified stack operations
int rjson_stream_push_val(rjson_stream* stream, rjson_value val);
int rjson_stream_push_kv(rjson_stream* stream, rjson_kv kv);
void rjson_stream_destroy(rjson_stream* stream);

#ifdef __cplusplus
}
#endif

#endif // RJSON_STREAM_H
