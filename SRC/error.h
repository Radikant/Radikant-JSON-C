#ifndef RJSON_ERROR_H
#define RJSON_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RJSON_OK = 0,
    
    // General / Argument Errors
    RJSON_ERROR_BAD_ARG = -1,     // Null pointer passed to API
    RJSON_ERROR_NOMEM = -2,       // Memory allocation failed (zone_alloc)

    // Parser Specific Errors
    RJSON_ERROR_PARSE_INCOMPLETE        = -10, // Stream ended unexpectedly
    RJSON_ERROR_PARSE_INVALID_FORMAT    = -11, // Syntax error / illegal character
    RJSON_ERROR_PARSE_DEPTH_EXCEEDED    = -12, // Recursion too deep (arrays/objects)
    RJSON_ERROR_PARSE_INVALID_NUMBER    = -13, // Malformed JSON number
    RJSON_ERROR_PARSE_INVALID_STRING    = -14, // Malformed JSON string (e.g. invalid escape)
    RJSON_ERROR_PARSE_TRAILING_GARBAGE  = -15, // Extra characters found after valid JSON object

    // Serializer Specific Errors
    RJSON_ERROR_SERIALIZE_NOMEM         = -30, // Failed to grow encoder buffer
    RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED= -31, // Object tree recursion too deep during encoding

    // Stream Specific Errors
    RJSON_ERROR_STREAM_BAD_ARG          = -40, // Passed a NULL stream or buffer to init

    // Object / Builder Specific Errors
    RJSON_ERROR_OBJECT_TYPE_MISMATCH    = -50, // Attempted to read an object as a type it is not
    RJSON_ERROR_OBJECT_OUT_OF_BOUNDS    = -51, // Attempted to access an array index outside its size
    RJSON_ERROR_OBJECT_NULL_POINTER     = -52  // Passed a NULL object pointer to an accessor
} rjson_error_t;

#ifdef __cplusplus
}
#endif

#endif // RJSON_ERROR_H