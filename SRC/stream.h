#ifndef RJSON_STREAM_H
#define RJSON_STREAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* data;
    size_t length;
    size_t position;
} rjson_stream;

void rjson_stream_init(rjson_stream* stream, const char* data, size_t length);
int rjson_stream_peek(rjson_stream* stream);
int rjson_stream_get(rjson_stream* stream);
void rjson_stream_skip_whitespace(rjson_stream* stream);
const char* rjson_stream_current_ptr(rjson_stream* stream);
size_t rjson_stream_remaining(rjson_stream* stream);

// For serialization (output stream)
typedef struct {
    char* buffer;
    size_t length;
    size_t capacity;
} rjson_out_stream;

int rjson_out_stream_init(rjson_out_stream* stream, size_t initial_capacity);
int rjson_out_stream_append(rjson_out_stream* stream, const char* data, size_t len);
void rjson_out_stream_destroy(rjson_out_stream* stream);

#ifdef __cplusplus
}
#endif

#endif // RJSON_STREAM_H
