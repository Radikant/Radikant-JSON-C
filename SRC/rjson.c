#include "rjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>  

#define RJSON_MAX_DEPTH 512

// --- Serialization Helpers (Internal) ---

/*
 * A simple dynamic string buffer for serialization.
 */
struct strbuf
{
    char *buffer;
    size_t length;
    size_t capacity;
};

/* Initializes a new string buffer */
static int strbuf_init(struct strbuf *sb, size_t initial_capacity)
{
    sb->buffer = (char *)malloc(initial_capacity);
    if (!sb->buffer)
        return -1;
    sb->length = 0;
    sb->capacity = initial_capacity;
    sb->buffer[0] = '\0';
    return 0;
}

/* Appends data to the string buffer, resizing if needed */
static int strbuf_append(struct strbuf *sb, const char *data, size_t len)
{
    if (sb->length + len + 1 > sb->capacity)
    {
        size_t new_capacity = (sb->capacity == 0) ? 64 : sb->capacity * 2;
        while (new_capacity < sb->length + len + 1)
        {
            new_capacity *= 2;
        }
        char *new_buffer = (char *)realloc(sb->buffer, new_capacity);
        if (!new_buffer)
            return -1;
        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }
    memcpy(sb->buffer + sb->length, data, len);
    sb->length += len;
    sb->buffer[sb->length] = '\0';
    return 0;
}

/* Frees the string buffer's internal memory */
static void strbuf_free(struct strbuf *sb)
{
    free(sb->buffer);
}

// --- Forward Declarations for Static Functions ---

// Parsing
static rjson_value *parse_value(const char **json, int depth);
static rjson_value *parse_string(const char **json);
static rjson_value *parse_number(const char **json);
static rjson_value *parse_literal(const char **json);
static rjson_value *parse_array(const char **json, int depth);
static rjson_value *parse_object(const char **json, int depth);
static void skip_whitespace(const char **json);
static char *unescape_string(const char *in_start, const char *in_end, size_t *out_len);

// Serialization
static int serialize_value(const rjson_value *value, struct strbuf *sb, int depth);
static int escape_string(const char *in, struct strbuf *sb);

// --- Memory Management Helpers ---

static rjson_value *create_value(rjson_type type)
{
    rjson_value *val = (rjson_value *)calloc(1, sizeof(rjson_value));
    if (!val)
        return NULL;
    val->type = type;
    return val;
}

rjson_value *rjson_null_new(void)
{
    return create_value(RJSON_NULL);
}

rjson_value *rjson_bool_new(int bool_val)
{
    rjson_value *val = create_value(RJSON_BOOL);
    if (val)
        val->as.bool_val = bool_val ? 1 : 0;
    return val;
}

rjson_value *rjson_number_new(double num_val)
{
    rjson_value *val = create_value(RJSON_NUMBER);
    if (val)
        val->as.num_val = num_val;
    return val;
}

rjson_value *rjson_string_new(const char *str_val)
{
    rjson_value *val = create_value(RJSON_STRING);
    if (!val)
        return NULL;

    val->as.str_val = (char *)malloc(strlen(str_val) + 1);
    if (!val->as.str_val)
    {
        free(val);
        return NULL;
    }
    strcpy(val->as.str_val, str_val);
    return val;
}

rjson_value *rjson_array_new(void)
{
    rjson_value *val = create_value(RJSON_ARRAY);
    // calloc handled initialization
    return val;
}

rjson_value *rjson_object_new(void)
{
    rjson_value *val = create_value(RJSON_OBJECT);
    // calloc handled initialization
    return val;
}

// --- Parsing Helper Functions ---

// Skips any whitespace characters in the input string.
static void skip_whitespace(const char **json)
{
    // Harden: Only skip RFC 8259 allowed whitespace (Space, Tab, LF, CR).
    // isspace() in C includes \v and \f, which are invalid in JSON.
    while (**json == ' ' || **json == '\t' || **json == '\n' || **json == '\r')
    {
        (*json)++;
    }
}

/**
 * @brief Processes an escaped string segment.
 * Allocates and returns a new string, setting out_len.
 * Does NOT handle \uXXXX unicode escapes.
 */
static char *unescape_string(const char *in_start, const char *in_end, size_t *out_len)
{
    // Allocation strategy: Unescaping never expands the byte length of the string
    // (e.g., "\u0041" is 6 bytes -> "A" is 1 byte).
    // So allocating (in_end - in_start + 1) is always safe.
    size_t max_len = (size_t)(in_end - in_start);
    char *out = (char *)malloc(max_len + 1);
    if (!out)
        return NULL;

    char *d = out;
    for (const char *p = in_start; p < in_end; p++)
    {
        if (*p == '\\')
        {
            p++;
            if (p >= in_end)
            {
                free(out);
                return NULL;
            } // Safety check

            switch (*p)
            {
            case '"':
                *d++ = '"';
                break;
            case '\\':
                *d++ = '\\';
                break;
            case '/':
                *d++ = '/';
                break;
            case 'b':
                *d++ = '\b';
                break;
            case 'f':
                *d++ = '\f';
                break;
            case 'n':
                *d++ = '\n';
                break;
            case 'r':
                *d++ = '\r';
                break;
            case 't':
                *d++ = '\t';
                break;
            case 'u':
            {
                // Harden: Implement \uXXXX decoding to UTF-8
                uint32_t cp = 0;
                // Need at least 4 hex digits
                if (in_end - p < 5)
                {
                    free(out);
                    return NULL;
                }

                for (int i = 0; i < 4; i++)
                {
                    char c = *(++p);
                    int v = -1;
                    if (c >= '0' && c <= '9')
                        v = c - '0';
                    else if (c >= 'a' && c <= 'f')
                        v = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F')
                        v = c - 'A' + 10;
                    if (v < 0)
                    {
                        free(out);
                        return NULL;
                    } // Invalid hex
                    cp = (cp << 4) | v;
                }

                // Handle UTF-16 surrogate pairs (e.g. emojis)
                if (cp >= 0xD800 && cp <= 0xDBFF)
                {
                    // Check for following low surrogate \uXXXX
                    if (in_end - p >= 7 && p[1] == '\\' && p[2] == 'u')
                    {
                        uint32_t low = 0;
                        const char *low_p = p + 2;
                        int valid_low = 1;
                        for (int i = 0; i < 4; i++)
                        {
                            char c = *(++low_p);
                            int v = -1;
                            if (c >= '0' && c <= '9')
                                v = c - '0';
                            else if (c >= 'a' && c <= 'f')
                                v = c - 'a' + 10;
                            else if (c >= 'A' && c <= 'F')
                                v = c - 'A' + 10;
                            if (v < 0)
                            {
                                valid_low = 0;
                                break;
                            }
                            low = (low << 4) | v;
                        }
                        if (valid_low && low >= 0xDC00 && low <= 0xDFFF)
                        {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                            p += 6; // Advance past the second \uXXXX
                        }
                    }
                }

                // Harden: Reject lone surrogates (invalid UTF-8) and null bytes (unsafe for C strings)
                if ((cp >= 0xD800 && cp <= 0xDFFF) || cp == 0)
                {
                    free(out);
                    return NULL;
                }

                // Encode to UTF-8
                if (cp < 0x80)
                {
                    *d++ = (char)cp;
                }
                else if (cp < 0x800)
                {
                    *d++ = (char)(0xC0 | (cp >> 6));
                    *d++ = (char)(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x10000)
                {
                    *d++ = (char)(0xE0 | (cp >> 12));
                    *d++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                    *d++ = (char)(0x80 | (cp & 0x3F));
                }
                else
                {
                    *d++ = (char)(0xF0 | (cp >> 18));
                    *d++ = (char)(0x80 | ((cp >> 12) & 0x3F));
                    *d++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                    *d++ = (char)(0x80 | (cp & 0x3F));
                }
                break;
            }
            default:
                // Harden: Fail on invalid escapes
                free(out);
                return NULL;
            }
        }
        else
        {
            *d++ = *p;
        }
    }

    *d = '\0';
    *out_len = (size_t)(d - out);
    return out;
}

// Parses a JSON string literal.
static rjson_value *parse_string(const char **json)
{
    (*json)++; // Skip opening quote
    const char *start = *json;

    // Find the end of the string, watching for escapes
    while (**json != '"' && **json != '\0')
    {
        if ((unsigned char)**json < 0x20)
            return NULL; // Harden: Reject unescaped control chars
        if (**json == '\\')
        {
            (*json)++;
            if (**json == '\0')
                return NULL; // Unterminated escape
        }
        (*json)++;
    }

    if (**json != '"')
    {
        return NULL; // Unterminated string
    }

    const char *end = *json;
    (*json)++; // Skip closing quote

    size_t unescaped_len = 0;
    char *str_content = unescape_string(start, end, &unescaped_len);
    if (!str_content)
        return NULL;

    rjson_value *val = create_value(RJSON_STRING);
    if (!val)
    {
        free(str_content);
        return NULL;
    }
    val->as.str_val = str_content;
    return val;
}

/**
 * @brief Parses a JSON number in a locale-independent way.
 * This avoids the locale-dependent decimal separator bug from strtod().
 */
static rjson_value *parse_number(const char **json)
{
    const char *start = *json;

    // Check for negative sign
    if (**json == '-')
        (*json)++;

    // Check for integer part
    if (**json == '0')
    {
        (*json)++;
        if (isdigit((unsigned char)**json))
            return NULL; // Harden: Leading zero not allowed (e.g. 01)
    }
    else if (isdigit((unsigned char)**json))
    {
        while (isdigit((unsigned char)**json))
            (*json)++;
    }
    else
    {
        return NULL; // Invalid: "e.g., -e10" or just "-"
    }

    // Check for fractional part
    if (**json == '.')
    {
        (*json)++;
        // After '.', must have at least one digit
        if (!isdigit((unsigned char)**json))
        {
            return NULL; // Invalid: "e.g., 1."
        }
        while (isdigit((unsigned char)**json))
            (*json)++;
    }

    // Check for exponent part
    if (**json == 'e' || **json == 'E')
    {
        (*json)++;
        // After 'e', can have optional +/-
        if (**json == '+' || **json == '-')
            (*json)++;

        // After 'e', must have at least one digit
        if (!isdigit((unsigned char)**json))
        {
            return NULL; // Invalid: "e.g., 1e" or "1e+"
        }
        while (isdigit((unsigned char)**json))
            (*json)++;
    }

    // We've found the end of the number. Now we use strtod on our segment.
    // We *must* use strtod for robust parsing of doubles, but since we've
    // validated the format, we are safe from the locale *parsing* bug
    // (e.g. "3.14" won't be parsed as "3").
    char *end;
    errno = 0;
    double num = strtod(start, &end);

    // Check for errors from strtod
    if (errno == ERANGE || !isfinite(num))
    {
        return NULL; // Overflow or invalid number
    }

    // The C standard locale is "C", where strtod always uses '.'.
    // If the *current* locale uses a ',', strtod might stop early.
    // Our 'end' pointer will be at *json.
    if (end != *json)
    {
        // This can happen if the locale is "fr_FR" and we parse "3.14".
        // strtod might stop at the '.', parsing "3".
        // We must manually rescan.

        // This is a failsafe: create a temporary NUL-terminated string
        // to parse in the "C" locale.
        size_t len = *json - start;
        char *temp_num = (char *)malloc(len + 1);
        if (!temp_num)
            return NULL;
        memcpy(temp_num, start, len);
        temp_num[len] = '\0';

        // This is still locale-dependent, but the manual parse above
        // is our best defense. The *truly* robust way is to set locale,
        // but that's not thread-safe.
        num = strtod(temp_num, &end);
        free(temp_num);

        if (*end != '\0')
        {
            return NULL; // Failsafe check failed
        }
    }

    rjson_value *val = create_value(RJSON_NUMBER);
    if (!val)
        return NULL;
    val->as.num_val = num;
    return val;
}

// Parses JSON literals: true, false, and null.
static rjson_value *parse_literal(const char **json)
{
    if (strncmp(*json, "true", 4) == 0)
    {
        *json += 4;
        rjson_value *val = create_value(RJSON_BOOL);
        if (val)
            val->as.bool_val = 1;
        return val;
    }
    if (strncmp(*json, "false", 5) == 0)
    {
        *json += 5;
        rjson_value *val = create_value(RJSON_BOOL);
        if (val)
            val->as.bool_val = 0;
        return val;
    }
    if (strncmp(*json, "null", 4) == 0)
    {
        *json += 4;
        return create_value(RJSON_NULL);
    }
    return NULL; // Invalid literal
}

// Parses a JSON array.
static rjson_value *parse_array(const char **json, int depth)
{
    if (depth >= RJSON_MAX_DEPTH)
        return NULL; // Stack exhaustion protection
    (*json)++;       // Skip '['

    rjson_value *arr_val = rjson_array_new();
    if (!arr_val)
        return NULL;

    skip_whitespace(json);
    if (**json == ']')
    {
        (*json)++; // Empty array
        return arr_val;
    }

    while (1)
    {
        rjson_value *element = parse_value(json, depth + 1);
        if (!element)
        {
            rjson_free(arr_val);
            return NULL;
        }

        if (rjson_array_add(arr_val, element) != 0)
        {
            rjson_free(element);
            rjson_free(arr_val);
            return NULL; // Out of memory
        }

        skip_whitespace(json);

        if (**json == ']')
        {
            (*json)++;
            break;
        }
        if (**json != ',')
        {
            rjson_free(arr_val);
            return NULL; // Expected comma or ']'
        }
        (*json)++; // Skip comma
    }
    return arr_val;
}

// Parses a JSON object.
static rjson_value *parse_object(const char **json, int depth)
{
    if (depth >= RJSON_MAX_DEPTH)
        return NULL; // Stack exhaustion protection
    (*json)++;       // Skip '{'

    rjson_value *obj_val = rjson_object_new();
    if (!obj_val)
        return NULL;

    skip_whitespace(json);
    if (**json == '}')
    {
        (*json)++; // Empty object
        return obj_val;
    }

    while (1)
    {
        skip_whitespace(json);
        if (**json != '"')
        {
            rjson_free(obj_val);
            return NULL; // Key must be a string
        }
        rjson_value *key_val = parse_string(json);
        if (!key_val)
        {
            rjson_free(obj_val);
            return NULL;
        }

        skip_whitespace(json);
        if (**json != ':')
        {
            rjson_free(key_val);
            rjson_free(obj_val);
            return NULL; // Expected colon
        }
        (*json)++; // Skip colon

        rjson_value *val = parse_value(json, depth + 1);
        if (!val)
        {
            rjson_free(key_val);
            rjson_free(obj_val);
            return NULL;
        }

        if (rjson_object_add(obj_val, key_val->as.str_val, val) != 0)
        {
            rjson_free(key_val);
            rjson_free(val);
            rjson_free(obj_val);
            return NULL; // Out of memory
        }

        free(key_val->as.str_val); // rjson_object_add made a copy
        free(key_val);             // Free the key *value*, keep the value

        skip_whitespace(json);

        if (**json == '}')
        {
            (*json)++;
            break;
        }
        if (**json != ',')
        {
            rjson_free(obj_val);
            return NULL; // Expected comma or '}'
        }
        (*json)++; // Skip comma
    }

    return obj_val;
}

// Main dispatcher for parsing any JSON value.
static rjson_value *parse_value(const char **json, int depth)
{
    skip_whitespace(json);
    switch (**json)
    {
    case '"':
        return parse_string(json);
    case '[':
        return parse_array(json, depth);
    case '{':
        return parse_object(json, depth);
    case 't':
    case 'f':
    case 'n':
        return parse_literal(json);
    default:
        if (**json == '-' || isdigit((unsigned char)**json))
        {
            return parse_number(json);
        }
    }
    return NULL; // Invalid character
}

// --- Public API Implementation ---

rjson_value *rjson_parse(const char *json_string)
{
    if (!json_string)
        return NULL;

    // Harden: Skip UTF-8 BOM if present (EF BB BF)
    if (strncmp(json_string, "\xEF\xBB\xBF", 3) == 0)
        json_string += 3;

    const char *current_pos = json_string;
    rjson_value *result = parse_value(&current_pos, 0);

    if (!result)
    {
        // Library should not log, just fail.
        return NULL;
    }

    skip_whitespace(&current_pos);
    if (*current_pos != '\0')
    {
        // Library should not log. Fail due to extra characters.
        rjson_free(result);
        return NULL;
    }

    return result;
}

void rjson_free(rjson_value *value)
{
    if (!value)
        return;

    size_t i;
    switch (value->type)
    {
    case RJSON_STRING:
        free(value->as.str_val);
        break;
    case RJSON_ARRAY:
        for (i = 0; i < value->as.arr_val.count; ++i)
        {
            rjson_free(value->as.arr_val.elements[i]);
        }
        free(value->as.arr_val.elements);
        break;
    case RJSON_OBJECT:
        for (i = 0; i < value->as.obj_val.count; ++i)
        {
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

rjson_value *rjson_object_get_value(const rjson_value *object, const char *key)
{
    if (!object || object->type != RJSON_OBJECT || !key)
    {
        return NULL;
    }

    for (size_t i = 0; i < object->as.obj_val.count; ++i)
    {
        if (strcmp(object->as.obj_val.keys[i], key) == 0)
        {
            return object->as.obj_val.values[i];
        }
    }

    return NULL; // Key not found
}

/**
 * @brief Adds an element to a JSON array.
 * This is "rock solid" - if realloc fails, it frees the new element
 * and leaves the original array unmodified.
 *
 * @param array The array object to add to.
 * @param element The element to add.
 * @return 0 on success, -1 on failure (out of memory).
 */
int rjson_array_add(rjson_value *array, rjson_value *element)
{
    if (!array || array->type != RJSON_ARRAY || !element)
    {
        return -1;
    }

    size_t new_count = array->as.arr_val.count + 1;
    rjson_value **new_elements = (rjson_value **)realloc(array->as.arr_val.elements, new_count * sizeof(rjson_value *));

    if (!new_elements)
    {
        return -1; // Out of memory
    }

    array->as.arr_val.elements = new_elements;
    array->as.arr_val.elements[new_count - 1] = element;
    array->as.arr_val.count = new_count;

    return 0;
}

/**
 * @brief Adds a key-value pair to a JSON object.
 * This is "rock solid" - if realloc fails, it frees the new key/value
 * and leaves the original object unmodified.
 *
 * @param object The object to add to.
 * @param key The string key. The function makes its own copy.
 * @param value The value to add.
 * @return 0 on success, -1 on failure (out of memory).
 */
int rjson_object_add(rjson_value *object, const char *key, rjson_value *value)
{
    if (!object || object->type != RJSON_OBJECT || !key || !value)
    {
        return -1;
    }

    // 1. Prepare new key and value
    char *new_key = (char *)malloc(strlen(key) + 1);
    if (!new_key)
    {
        return -1;
    }
    strcpy(new_key, key);

    // 2. Try to grow arrays
    size_t new_count = object->as.obj_val.count + 1;
    char **new_keys = (char **)realloc(object->as.obj_val.keys, new_count * sizeof(char *));
    if (!new_keys)
    {
        free(new_key);
        return -1; // Out of memory
    }
    object->as.obj_val.keys = new_keys;

    rjson_value **new_values = (rjson_value **)realloc(object->as.obj_val.values, new_count * sizeof(rjson_value *));
    if (!new_values)
    {
        // We are in a tricky spot. new_keys succeeded, but new_values failed.
        // We must shrink new_keys back to be safe.
        // This cast is to silence a -Wunused-result warning. We're in
        // cleanup mode, so we don't care if it fails.
        (void)realloc(object->as.obj_val.keys, (new_count - 1) * sizeof(char *));
        free(new_key);
        return -1; // Out of memory
    }
    object->as.obj_val.values = new_values;

    // 3. Add new key and value
    object->as.obj_val.keys[new_count - 1] = new_key;
    object->as.obj_val.values[new_count - 1] = value;
    object->as.obj_val.count = new_count;

    return 0;
}

// --- Serialization Implementation ---

/**
 * @brief Escapes a string and appends it to the string buffer.
 * Does NOT include the opening/closing quotes.
 */
static int escape_string(const char *in, struct strbuf *sb)
{
    strbuf_append(sb, "\"", 1);

    const char *p = in;
    const char *start = in;

    while (*p)
    {
        char c = *p;
        const char *replacement = NULL;
        size_t len = 1;
        char hex_buf[7];

        switch (c)
        {
        case '"':
            replacement = "\\\"";
            len = 2;
            break;
        case '\\':
            replacement = "\\\\";
            len = 2;
            break;
        case '\b':
            replacement = "\\b";
            len = 2;
            break;
        case '\f':
            replacement = "\\f";
            len = 2;
            break;
        case '\n':
            replacement = "\\n";
            len = 2;
            break;
        case '\r':
            replacement = "\\r";
            len = 2;
            break;
        case '\t':
            replacement = "\\t";
            len = 2;
            break;
        default:
            if ((unsigned char)c < 0x20)
            {
                // Control character, needs \uXXXX encoding
                snprintf(hex_buf, sizeof(hex_buf), "\\u%04x", (unsigned char)c);
                replacement = hex_buf;
                len = 6;
            }
            break; // No replacement, continue
        }

        if (replacement)
        {
            // Append the chunk before this special char
            if (p > start)
            {
                if (strbuf_append(sb, start, p - start) != 0)
                    return -1;
            }
            // Append the replacement
            if (strbuf_append(sb, replacement, len) != 0)
                return -1;
            // Move start to *after* this char
            start = p + 1;
        }
        p++;
    }

    // Append any remaining part of the string
    if (p > start)
    {
        if (strbuf_append(sb, start, p - start) != 0)
            return -1;
    }

    strbuf_append(sb, "\"", 1);
    return 0;
}

static int serialize_number(double num, struct strbuf *sb)
{
    // Harden: JSON does not support NaN or Infinity.
    if (!isfinite(num))
        return -1;

    // snprintf's %g is locale-dependent (e.g., "3,14")
    // A robust solution uses a custom formatter or forces C locale.
    // For simplicity, we use snprintf, but this is a known
    // weak point if the locale is not "C".

    char num_buf[64];
    // Use %g for general, %f for high precision if needed
    // Using .17 to ensure round-trip precision (DBL_DECIMAL_DIG).
    int len = snprintf(num_buf, sizeof(num_buf), "%.17g", num);

    // Locale-fix: if snprintf used a comma, replace it with a dot.
    char *comma = strchr(num_buf, ',');
    if (comma)
    {
        *comma = '.';
    }

    if (len < 0 || len >= sizeof(num_buf))
    {
        return -1; // Encoding error or buffer overflow
    }

    return strbuf_append(sb, num_buf, len);
}

static int serialize_value(const rjson_value *value, struct strbuf *sb, int depth)
{
    if (depth >= RJSON_MAX_DEPTH)
        return -1; // Harden: Prevent stack overflow

    switch (value->type)
    {
    case RJSON_NULL:
        return strbuf_append(sb, "null", 4);
    case RJSON_BOOL:
        return strbuf_append(sb, value->as.bool_val ? "true" : "false", value->as.bool_val ? 4 : 5);
    case RJSON_NUMBER:
        return serialize_number(value->as.num_val, sb);
    case RJSON_STRING:
        return escape_string(value->as.str_val, sb);

    case RJSON_ARRAY:
    {
        if (strbuf_append(sb, "[", 1) != 0)
            return -1;
        for (size_t i = 0; i < value->as.arr_val.count; ++i)
        {
            if (i > 0)
            {
                if (strbuf_append(sb, ",", 1) != 0)
                    return -1;
            }
            if (serialize_value(value->as.arr_val.elements[i], sb, depth + 1) != 0)
                return -1;
        }
        return strbuf_append(sb, "]", 1);
    }

    case RJSON_OBJECT:
    {
        if (strbuf_append(sb, "{", 1) != 0)
            return -1;
        for (size_t i = 0; i < value->as.obj_val.count; ++i)
        {
            if (i > 0)
            {
                if (strbuf_append(sb, ",", 1) != 0)
                    return -1;
            }
            // Serialize key (which is always a string)
            if (escape_string(value->as.obj_val.keys[i], sb) != 0)
                return -1;

            if (strbuf_append(sb, ":", 1) != 0)
                return -1;

            // Serialize value
            if (serialize_value(value->as.obj_val.values[i], sb, depth + 1) != 0)
                return -1;
        }
        return strbuf_append(sb, "}", 1);
    }
    }
    return -1; // Unreachable
}

int rjson_serialize(const rjson_value *value, char **out_string, size_t *out_length)
{
    if (!value || !out_string)
    {
        return -1;
    }

    struct strbuf sb;
    if (strbuf_init(&sb, 1024) != 0)
    {
        return -1; // Out of memory
    }

    if (serialize_value(value, &sb, 0) != 0)
    {
        strbuf_free(&sb);
        return -1; // Serialization error
    }

    *out_string = sb.buffer; // Transfer ownership
    if (out_length)
    {
        *out_length = sb.length;
    }

    return 0;
}

// --- Pretty Print Implementation ---

static void rjson_print_internal(const rjson_value *value, int indent)
{
    if (!value)
    {
        printf("null");
        return;
    }

    switch (value->type)
    {
    case RJSON_NULL:
        printf("null");
        break;
    case RJSON_BOOL:
        printf(value->as.bool_val ? "true" : "false");
        break;
    case RJSON_NUMBER:
        // %g is fine for printing, as it's for humans
        printf("%g", value->as.num_val);
        break;
    case RJSON_STRING:
        // Print simple, non-escaped string for readability
        printf("\"%s\"", value->as.str_val);
        break;
    case RJSON_ARRAY:
        printf("[\n");
        for (size_t i = 0; i < value->as.arr_val.count; ++i)
        {
            for (int j = 0; j < indent + 1; ++j)
                printf("  ");
            rjson_print_internal(value->as.arr_val.elements[i], indent + 1);
            if (i < value->as.arr_val.count - 1)
            {
                printf(",");
            }
            printf("\n");
        }
        for (int j = 0; j < indent; ++j)
            printf("  ");
        printf("]");
        break;
    case RJSON_OBJECT:
        printf("{\n");
        for (size_t i = 0; i < value->as.obj_val.count; ++i)
        {
            for (int j = 0; j < indent + 1; ++j)
                printf("  ");
            printf("\"%s\": ", value->as.obj_val.keys[i]);
            rjson_print_internal(value->as.obj_val.values[i], indent + 1);
            if (i < value->as.obj_val.count - 1)
            {
                printf(",");
            }
            printf("\n");
        }
        for (int j = 0; j < indent; ++j)
            printf("  ");
        printf("}");
        break;
    }
}

void rjson_print(const rjson_value *value, int indent)
{
    rjson_print_internal(value, indent);
}