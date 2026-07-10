#ifndef RJSON_DECODE_H
#define RJSON_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zone.h"
#include "stream.h"
#include "object.h"
#include "error.h"

/**
 * @brief Parses a JSON tree from a stream using the provided zone for memory.
 * 
 * @param zone The arena allocator to use.
 * @param stream The input stream to parse from.
 * @param out_value Pointer to store the root value of the parsed JSON tree.
 * @return RJSON_OK on success, or an appropriate error code.
 */
rjson_error_t rjson_decode_stream(rjson_zone* zone, rjson_stream* stream, rjson_value* out_value);

#ifdef __cplusplus
}
#endif

#endif // RJSON_DECODE_H
