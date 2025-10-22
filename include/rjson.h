#ifndef RJSON_H
#define RJSON_H

#include <stddef.h> // For size_t

// --- Type Definitions ---

// Enum to represent the type of a JSON value.
typedef enum {
    RJSON_NULL,
    RJSON_BOOL,
    RJSON_NUMBER,
    RJSON_STRING,
    RJSON_ARRAY,
    RJSON_OBJECT
} rjson_type;

// Forward declaration of the main JSON value struct.
typedef struct rjson_value rjson_value;

// Structure to represent a single JSON value.
// It uses a tagged union to store data of different types.
struct rjson_value {
    rjson_type type;
    union {
        int bool_val;      // For RJSON_BOOL
        double num_val;    // For RJSON_NUMBER
        char* str_val;     // For RJSON_STRING
        struct {           // For RJSON_ARRAY
            rjson_value** elements;
            size_t count;
        } arr_val;
        struct {           // For RJSON_OBJECT
            char** keys;
            rjson_value** values;
            size_t count;
        } obj_val;
    } as;
};

// --- Public API Functions ---

/**
 * @brief Parses a null-terminated JSON string into an rjson_value structure.
 *
 * @param json_string The input JSON string to parse.
 * @return A pointer to a newly allocated rjson_value structure representing the
 * parsed JSON, or NULL if parsing fails. The caller is responsible for

 * freeing this structure using rjson_free().
 */
rjson_value* rjson_parse(const char* json_string);

/**
 * @brief Frees the memory allocated for an rjson_value structure and all its children.
 *
 * @param value A pointer to the rjson_value to free.
 */
void rjson_free(rjson_value* value);

/**
 * @brief A utility function to print the structure of a parsed JSON value.
 * Useful for debugging.
 *
 * @param value The rjson_value to print.
 * @param indent The initial indentation level.
 */
void rjson_print(const rjson_value* value, int indent);

/**
 * @brief Retrieves a value from a JSON object by its key.
 *
 * @param object A pointer to an rjson_value of type RJSON_OBJECT.
 * @param key The null-terminated string key to search for.
 * @return A pointer to the rjson_value associated with the key,
 * or NULL if the key is not found or the input value is not an object.
 */
rjson_value* rjson_object_get_value(const rjson_value* object, const char* key);

#endif // RJSON_H