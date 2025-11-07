#ifndef RJSON_H
#define RJSON_H

#include <stddef.h>

// --- Type Definitions ---

typedef enum {
    RJSON_NULL,
    RJSON_BOOL,
    RJSON_NUMBER,
    RJSON_STRING,
    RJSON_ARRAY,
    RJSON_OBJECT
} rjson_type;

struct rjson_value; // Forward declaration

typedef struct {
    struct rjson_value** elements;
    size_t count;
} rjson_array;

typedef struct {
    char** keys;
    struct rjson_value** values;
    size_t count;
} rjson_object;

typedef struct rjson_value {
    rjson_type type;
    union {
        int bool_val;
        double num_val;
        char* str_val;
        rjson_array arr_val;
        rjson_object obj_val;
    } as;
} rjson_value;

// --- Public API ---

/**
 * @brief Parses a NUL-terminated JSON string into a tree of rjson_value nodes.
 *
 * @param json_string The JSON string to parse.
 * @return A pointer to the root rjson_value, or NULL on failure.
 */
rjson_value* rjson_parse(const char* json_string);

/**
 * @brief Serializes a tree of rjson_value nodes into a compact JSON string.
 *
 * @param value The root rjson_value to serialize.
 * @param out_string A pointer to a char* which will be allocated and filled
 * with the NUL-terminated JSON string. The caller
 * is responsible for freeing this with `free()`.
 * @param out_len A pointer to a size_t to store the length (optional, can be NULL).
 * @return 0 on success, -1 on failure (e.g., OOM).
 */
int rjson_serialize(const rjson_value* value, char** out_string, size_t* out_len);

/**
 * @brief Frees an rjson_value and all its children recursively.
 *
 * @param value The rjson_value to free.
 */
void rjson_free(rjson_value* value);

/**
 * @brief Retrieves a value from an RJSON_OBJECT by its key.
 *
 * @param object A pointer to an rjson_value of type RJSON_OBJECT.
 * @param key The NUL-terminated string key to search for.
 * @return A pointer to the corresponding rjson_value, or NULL if not found.
 */
rjson_value* rjson_object_get_value(const rjson_value* object, const char* key);

/**
 * @brief Prints a formatted representation of an rjson_value to stdout.
 *
 * @param value The rjson_value to print.
 * @param indent The initial indentation level.
 */
void rjson_print(const rjson_value* value, int indent);


// --- JSON Construction Helpers ---

/**
 * @brief Creates a new, empty RJSON_OBJECT.
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_object_new(void);

/**
 * @brief Creates a new, empty RJSON_ARRAY.
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_array_new(void);

/**
 * @brief Creates a new RJSON_STRING from a C string.
 * @param s The string to copy.
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_string_new(const char* s);

/**
 * @brief Creates a new RJSON_NUMBER.
 * @param n The number.
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_number_new(double n);

/**
 * @brief Creates a new RJSON_BOOL.
 * @param b The boolean value (0 for false, non-zero for true).
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_bool_new(int b);

/**
 * @brief Creates a new RJSON_NULL.
 * @return A pointer to the new rjson_value, or NULL on failure.
 */
rjson_value* rjson_null_new(void);

/**
 * @brief Adds a key-value pair to an RJSON_OBJECT.
 * The key is copied. The value is "donated" and will be freed
 * when the parent object is freed.
 *
 * @param object The RJSON_OBJECT to modify.
 * @param key The string key.
 * @param value The rjson_value to add.
 * @return 0 on success, -1 on failure.
 */
int rjson_object_add(rjson_value* object, const char* key, rjson_value* value);

/**
 * @brief Adds an element to an RJSON_ARRAY.
 * The value is "donated" and will be freed when the parent array is freed.
 *
 * @param array The RJSON_ARRAY to modify.
 * @param value The rjson_value to add.
 * @return 0 on success, -1 on failure.
 */
int rjson_array_add(rjson_value* array, rjson_value* value);


#endif // RJSON_H
