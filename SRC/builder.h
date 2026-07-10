#ifndef RJSON_BUILDER_H
#define RJSON_BUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stream.h"
#include "error.h"
#include <stddef.h>

#define RJSON_BUILDER_MAX_DEPTH 256

/**
 * @brief Streaming JSON Builder.
 * Allows constructing JSON strings directly into an output stream without allocating an AST.
 */
typedef struct {
    rjson_out_stream* stream;
    int depth;
    int expects_value[RJSON_BUILDER_MAX_DEPTH]; // True if a key was just written
    int needs_comma[RJSON_BUILDER_MAX_DEPTH];   // True if the next element requires a preceding comma
} rjson_builder;

rjson_error_t rjson_builder_init(rjson_builder* builder, rjson_out_stream* stream);

rjson_error_t rjson_builder_start_object(rjson_builder* builder);
rjson_error_t rjson_builder_end_object(rjson_builder* builder);

rjson_error_t rjson_builder_start_array(rjson_builder* builder);
rjson_error_t rjson_builder_end_array(rjson_builder* builder);

rjson_error_t rjson_builder_write_key(rjson_builder* builder, const char* key, size_t len);
rjson_error_t rjson_builder_write_string(rjson_builder* builder, const char* str, size_t len);
rjson_error_t rjson_builder_write_number(rjson_builder* builder, double num);
rjson_error_t rjson_builder_write_bool(rjson_builder* builder, int val);
rjson_error_t rjson_builder_write_null(rjson_builder* builder);

#ifdef __cplusplus
}
#endif

#endif // RJSON_BUILDER_H
