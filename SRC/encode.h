#ifndef RJSON_ENCODE_H
#define RJSON_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stream.h"
#include "object.h"
#include "error.h"

/**
 * @brief Serializes a JSON AST into the provided output stream.
 * 
 * @param stream The output stream buffer to write to.
 * @param value The root JSON node to serialize.
 * @return RJSON_OK on success, or an appropriate error code.
 */
rjson_error_t rjson_encode_stream(rjson_out_stream* stream, const rjson_value* value);

#ifdef __cplusplus
}
#endif

#endif // RJSON_ENCODE_H
