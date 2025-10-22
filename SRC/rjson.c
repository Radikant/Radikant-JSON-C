#include "rjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// --- Forward Declarations for Static Helper Functions ---

static rjson_value* parse_value(const char** json);
static void skip_whitespace(const char** json);

// --- Memory Management Helpers ---

static rjson_value* create_value(rjson_type type) {
    rjson_value* val = (rjson_value*)malloc(sizeof(rjson_value));
    if (!val) return NULL;
    val->type = type;
    return val;
}

// --- Parsing Helper Functions ---

// Skips any whitespace characters in the input string.
static void skip_whitespace(const char** json) {
    while (isspace((unsigned char)**json)) {
        (*json)++;
    }
}

// Parses a JSON string literal.
static rjson_value* parse_string(const char** json) {
    (*json)++; // Skip opening quote
    const char* start = *json;
    while (**json != '"' && **json != '\0') {
        // Basic implementation: does not handle escape sequences yet
        (*json)++;
    }

    if (**json != '"') {
        return NULL; // Unterminated string
    }

    size_t len = *json - start;
    char* str_content = (char*)malloc(len + 1);
    if (!str_content) return NULL;

    memcpy(str_content, start, len);
    str_content[len] = '\0';
    (*json)++; // Skip closing quote

    rjson_value* val = create_value(RJSON_STRING);
    if (!val) {
        free(str_content);
        return NULL;
    }
    val->as.str_val = str_content;
    return val;
}

// Parses a JSON number literal.
static rjson_value* parse_number(const char** json) {
    char* end;
    double num = strtod(*json, &end);

    if (end == *json) {
        return NULL; // Not a number
    }
    
    *json = end;

    rjson_value* val = create_value(RJSON_NUMBER);
    if (!val) return NULL;
    val->as.num_val = num;
    return val;
}

// Parses JSON literals: true, false, and null.
static rjson_value* parse_literal(const char** json) {
    if (strncmp(*json, "true", 4) == 0) {
        *json += 4;
        rjson_value* val = create_value(RJSON_BOOL);
        if (!val) return NULL;
        val->as.bool_val = 1;
        return val;
    }
    if (strncmp(*json, "false", 5) == 0) {
        *json += 5;
        rjson_value* val = create_value(RJSON_BOOL);
        if (!val) return NULL;
        val->as.bool_val = 0;
        return val;
    }
    if (strncmp(*json, "null", 4) == 0) {
        *json += 4;
        return create_value(RJSON_NULL);
    }
    return NULL; // Invalid literal
}

// Parses a JSON array.
static rjson_value* parse_array(const char** json) {
    (*json)++; // Skip '['
    
    rjson_value* arr_val = create_value(RJSON_ARRAY);
    if (!arr_val) return NULL;
    arr_val->as.arr_val.elements = NULL;
    arr_val->as.arr_val.count = 0;

    skip_whitespace(json);
    if (**json == ']') {
        (*json)++; // Empty array
        return arr_val;
    }

    while (1) {
        rjson_value* element = parse_value(json);
        if (!element) {
            rjson_free(arr_val);
            return NULL;
        }

        // Add element to the array
        arr_val->as.arr_val.count++;
        void* new_ptr = realloc(arr_val->as.arr_val.elements, arr_val->as.arr_val.count * sizeof(rjson_value*));
        if (!new_ptr) {
            free(element);
            rjson_free(arr_val);
            return NULL;
        }
        arr_val->as.arr_val.elements = (rjson_value**)new_ptr;
        arr_val->as.arr_val.elements[arr_val->as.arr_val.count - 1] = element;
        
        skip_whitespace(json);

        if (**json == ']') {
            (*json)++;
            break;
        }
        if (**json != ',') {
            rjson_free(arr_val);
            return NULL; // Expected comma or ']'
        }
        (*json)++; // Skip comma
    }
    return arr_val;
}

// Parses a JSON object.
static rjson_value* parse_object(const char** json) {
    (*json)++; // Skip '{'

    rjson_value* obj_val = create_value(RJSON_OBJECT);
    if (!obj_val) return NULL;
    obj_val->as.obj_val.keys = NULL;
    obj_val->as.obj_val.values = NULL;
    obj_val->as.obj_val.count = 0;

    skip_whitespace(json);
    if (**json == '}') {
        (*json)++; // Empty object
        return obj_val;
    }

    while (1) {
        skip_whitespace(json);
        if (**json != '"') {
             rjson_free(obj_val);
             return NULL; // Key must be a string
        }
        rjson_value* key_val = parse_string(json);
        if (!key_val) {
            rjson_free(obj_val);
            return NULL;
        }

        skip_whitespace(json);
        if (**json != ':') {
            rjson_free(key_val);
            rjson_free(obj_val);
            return NULL; // Expected colon
        }
        (*json)++; // Skip colon

        rjson_value* val = parse_value(json);
        if (!val) {
            rjson_free(key_val);
            rjson_free(obj_val);
            return NULL;
        }

        // Add key-value pair to object
        obj_val->as.obj_val.count++;
        void* new_keys = realloc(obj_val->as.obj_val.keys, obj_val->as.obj_val.count * sizeof(char*));
        void* new_vals = realloc(obj_val->as.obj_val.values, obj_val->as.obj_val.count * sizeof(rjson_value*));

        if (!new_keys || !new_vals) {
            rjson_free(key_val);
            rjson_free(val);
            rjson_free(obj_val);
            return NULL;
        }

        obj_val->as.obj_val.keys = (char**)new_keys;
        obj_val->as.obj_val.values = (rjson_value**)new_vals;

        obj_val->as.obj_val.keys[obj_val->as.obj_val.count - 1] = key_val->as.str_val;
        obj_val->as.obj_val.values[obj_val->as.obj_val.count - 1] = val;
        free(key_val); // Free the container, keep the string

        skip_whitespace(json);

        if (**json == '}') {
            (*json)++;
            break;
        }
        if (**json != ',') {
            rjson_free(obj_val);
            return NULL; // Expected comma or '}'
        }
        (*json)++; // Skip comma
    }

    return obj_val;
}

// Main dispatcher for parsing any JSON value.
static rjson_value* parse_value(const char** json) {
    skip_whitespace(json);
    switch (**json) {
        case '"':  return parse_string(json);
        case '[':  return parse_array(json);
        case '{':  return parse_object(json);
        case 't':
        case 'f':
        case 'n':  return parse_literal(json);
        default:
            if (**json == '-' || isdigit((unsigned char)**json)) {
                return parse_number(json);
            }
    }
    return NULL; // Invalid character
}

// --- Public API Implementation ---

rjson_value* rjson_parse(const char* json_string) {
    const char* current_pos = json_string;
    rjson_value* result = parse_value(&current_pos);
    
    if (!result) {
        fprintf(stderr, "Error: Failed to parse JSON string.\n");
        return NULL;
    }

    skip_whitespace(&current_pos);
    if (*current_pos != '\0') {
        fprintf(stderr, "Error: Extra characters at end of JSON string.\n");
        rjson_free(result);
    return NULL;
  }

  return result;
}

void rjson_free(rjson_value* value) {
    if (!value) return;

    size_t i;
    switch (value->type) {
        case RJSON_STRING:
            free(value->as.str_val);
            break;
        case RJSON_ARRAY:
            for (i = 0; i < value->as.arr_val.count; ++i) {
                rjson_free(value->as.arr_val.elements[i]);
            }
            free(value->as.arr_val.elements);
            break;
        case RJSON_OBJECT:
            for (i = 0; i < value->as.obj_val.count; ++i) {
                free(value->as.obj_val.keys[i]);
                rjson_free(value->as.obj_val.values[i]);
            }
            free(value->as.obj_val.keys);
            free(value->as.obj_val.values);
            break;
        default:
            // No dynamic memory for NULL, BOOL, NUMBER
            break;
    }
    free(value);
}

rjson_value* rjson_object_get_value(const rjson_value* object, const char* key) {
    if (!object || object->type != RJSON_OBJECT || !key) {
        return NULL;
    }

    for (size_t i = 0; i < object->as.obj_val.count; ++i) {
        if (strcmp(object->as.obj_val.keys[i], key) == 0) {
            return object->as.obj_val.values[i];
        }
    }

    return NULL; // Key not found
}

void rjson_print(const rjson_value* value, int indent) {
  if (!value) {
    printf("null\n");
        return;
    }

    char indent_str[256] = {0};
    for (int i = 0; i < indent; ++i) strcat(indent_str, "  ");

    switch (value->type) {
        case RJSON_NULL:
            printf("null");
            break;
        case RJSON_BOOL:
            printf(value->as.bool_val ? "true" : "false");
            break;
        case RJSON_NUMBER:
            printf("%g", value->as.num_val);
            break;
        case RJSON_STRING:
            printf("\"%s\"", value->as.str_val);
            break;
        case RJSON_ARRAY:
            printf("[\n");
            for (size_t i = 0; i < value->as.arr_val.count; ++i) {
                printf("%s  ", indent_str);
                rjson_print(value->as.arr_val.elements[i], indent + 1);
                if (i < value->as.arr_val.count - 1) {
                    printf(",");
                }
                printf("\n");
            }
            printf("%s]", indent_str);
            break;
        case RJSON_OBJECT:
            printf("{\n");
            for (size_t i = 0; i < value->as.obj_val.count; ++i) {
                printf("%s  \"%s\": ", indent_str, value->as.obj_val.keys[i]);
                rjson_print(value->as.obj_val.values[i], indent + 1);
                if (i < value->as.obj_val.count - 1) {
                    printf(",");
                }
                printf("\n");
            }
            printf("%s}", indent_str);
            break;
    }
}