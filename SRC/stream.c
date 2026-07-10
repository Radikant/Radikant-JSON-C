#include "stream.h"
#include <stdlib.h>
#include <string.h>

void rjson_stream_init(rjson_stream* stream, const char* data, size_t length) {
    stream->data = data;
    stream->length = length;
    stream->position = 0;
}

int rjson_stream_peek(rjson_stream* stream) {
    if (stream->position >= stream->length) return -1;
    return (unsigned char)stream->data[stream->position];
}

int rjson_stream_get(rjson_stream* stream) {
    if (stream->position >= stream->length) return -1;
    return (unsigned char)stream->data[stream->position++];
}

void rjson_stream_skip_whitespace(rjson_stream* stream) {
    while (stream->position < stream->length) {
        char c = stream->data[stream->position];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            stream->position++;
        } else {
            break;
        }
    }
}

const char* rjson_stream_current_ptr(rjson_stream* stream) {
    return stream->data + stream->position;
}

size_t rjson_stream_remaining(rjson_stream* stream) {
    return stream->length - stream->position;
}

int rjson_out_stream_init(rjson_out_stream* stream, size_t initial_capacity) {
    stream->buffer = (char*)malloc(initial_capacity);
    if (!stream->buffer) return -1;
    stream->length = 0;
    stream->capacity = initial_capacity;
    stream->buffer[0] = '\0';
    return 0;
}

int rjson_out_stream_append(rjson_out_stream* stream, const char* data, size_t len) {
    if (stream->length + len + 1 > stream->capacity) {
        size_t new_capacity = stream->capacity == 0 ? 64 : stream->capacity * 2;
        while (new_capacity < stream->length + len + 1) {
            new_capacity *= 2;
        }
        char* new_buffer = (char*)realloc(stream->buffer, new_capacity);
        if (!new_buffer) return -1;
        stream->buffer = new_buffer;
        stream->capacity = new_capacity;
    }
    memcpy(stream->buffer + stream->length, data, len);
    stream->length += len;
    stream->buffer[stream->length] = '\0';
    return 0;
}

void rjson_out_stream_destroy(rjson_out_stream* stream) {
    free(stream->buffer);
    stream->buffer = NULL;
}
