#ifndef RJSON_H
#define RJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "error.h"
#include "builder.h"
#include "object.h"
#include "zone.h"
#include <stddef.h>

// A Document owns the parsed JSON AST and its underlying memory arena
typedef struct {
    rjson_zone* zone;
    rjson_value* root;
    rjson_error_t error;
} rjson_doc;

/**
 * @brief Parses a NUL-terminated JSON string into a document.
 * 
 * @param json_string The JSON string to parse.
 * @return An rjson_doc. Check doc.error == RJSON_OK to ensure success.
 */
rjson_doc rjson_parse(const char* json_string);

/**
 * @brief Parses a JSON string of a specific length into a document.
 * 
 * @param json_string The JSON string to parse.
 * @param length The length of the string.
 * @return An rjson_doc. Check doc.error == RJSON_OK to ensure success.
 */
rjson_doc rjson_parse_with_length(const char* json_string, size_t length);

/**
 * @brief Serializes a tree of rjson_value nodes into a compact JSON string.
 * 
 * @param value The root rjson_value to serialize.
 * @param out_string Pointer to a char* which will be allocated and filled. The caller must free().
 * @param out_len Pointer to store the length (optional).
 * @return 0 on success, -1 on failure.
 */
int rjson_serialize(const rjson_value* value, char** out_string, size_t* out_len);

/**
 * @brief Frees the entire JSON document (destroys the zone).
 */
void rjson_free(rjson_doc* doc);

/**
 * @brief Retrieves a value from an RJSON_OBJECT by its key (NUL-terminated).
 */
rjson_value* rjson_object_get_value(const rjson_value* object, const char* key);

/**
 * @brief Prints a formatted representation of an rjson_value to stdout.
 */
void rjson_print(const rjson_value* value, int indent);

#ifdef __cplusplus
}
#endif

#endif // RJSON_H
