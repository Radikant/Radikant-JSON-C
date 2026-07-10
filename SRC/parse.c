#include "parse.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define RJSON_MAX_DEPTH 512

static rjson_error_t parse_value(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value** out_value);

// Helper to decode unicode hex
static int decode_hex4(const char* p, uint32_t* out_cp) {
    uint32_t cp = 0;
    for (int i = 0; i < 4; i++) {
        char c = p[i];
        int v = -1;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        if (v < 0) return -1;
        cp = (cp << 4) | v;
    }
    *out_cp = cp;
    return 0;
}

static rjson_error_t parse_string(rjson_zone* zone, rjson_stream* stream, rjson_value** out_value) {
    rjson_stream_get(stream); // skip '"'
    
    size_t start_pos = stream->position;
    int has_escapes = 0;
    
    // Fast scan
    while (1) {
        if (stream->position >= stream->length) return RJSON_ERROR_PARSE_INCOMPLETE;
        char c = stream->data[stream->position];
        if (c == '"') break;
        if (c == '\\') {
            has_escapes = 1;
            stream->position++; // skip the backslash
            if (stream->position >= stream->length) return RJSON_ERROR_PARSE_INCOMPLETE;
            // the escaped character is skipped by the stream->position++ at the end of the loop
        } else if ((unsigned char)c < 0x20) {
            return RJSON_ERROR_PARSE_INVALID_STRING;
        }
        stream->position++;
    }
    
    size_t string_len = stream->position - start_pos;
    const char* string_start = stream->data + start_pos;
    
    stream->position++; // skip closing '"'
    
    rjson_value* val = rjson_string_new(zone, NULL, 0);
    if (!val) return RJSON_ERROR_NOMEM;
    
    if (!has_escapes) {
        // Zero-copy fast path!
        val->as.str_val.ptr = string_start;
        val->as.str_val.len = string_len;
        *out_value = val;
        return RJSON_OK;
    }
    
    // Slow path: allocate from zone and unescape
    char* dest = (char*)rjson_zone_alloc(zone, string_len + 1);
    if (!dest) return RJSON_ERROR_NOMEM;
    
    size_t d_idx = 0;
    for (size_t i = 0; i < string_len; i++) {
        char c = string_start[i];
        if (c == '\\') {
            i++;
            if (i >= string_len) return RJSON_ERROR_PARSE_INVALID_STRING;
            char e = string_start[i];
            switch (e) {
                case '"': dest[d_idx++] = '"'; break;
                case '\\': dest[d_idx++] = '\\'; break;
                case '/': dest[d_idx++] = '/'; break;
                case 'b': dest[d_idx++] = '\b'; break;
                case 'f': dest[d_idx++] = '\f'; break;
                case 'n': dest[d_idx++] = '\n'; break;
                case 'r': dest[d_idx++] = '\r'; break;
                case 't': dest[d_idx++] = '\t'; break;
                case 'u': {
                    if (i + 4 >= string_len) return RJSON_ERROR_PARSE_INVALID_STRING;
                    uint32_t cp;
                    if (decode_hex4(string_start + i + 1, &cp) != 0) return RJSON_ERROR_PARSE_INVALID_STRING;
                    i += 4;
                    
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        // surrogate pair
                        if (i + 6 < string_len && string_start[i+1] == '\\' && string_start[i+2] == 'u') {
                            uint32_t low;
                            if (decode_hex4(string_start + i + 3, &low) == 0 && low >= 0xDC00 && low <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                                i += 6;
                            }
                        }
                    }
                    
                    // Reject lone surrogates to ensure valid UTF-8 output
                    if (cp >= 0xD800 && cp <= 0xDFFF) return RJSON_ERROR_PARSE_INVALID_STRING; 
                    
                    if (cp < 0x80) {
                        dest[d_idx++] = (char)cp;
                    } else if (cp < 0x800) {
                        dest[d_idx++] = (char)(0xC0 | (cp >> 6));
                        dest[d_idx++] = (char)(0x80 | (cp & 0x3F));
                    } else if (cp < 0x10000) {
                        dest[d_idx++] = (char)(0xE0 | (cp >> 12));
                        dest[d_idx++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        dest[d_idx++] = (char)(0x80 | (cp & 0x3F));
                    } else {
                        dest[d_idx++] = (char)(0xF0 | (cp >> 18));
                        dest[d_idx++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                        dest[d_idx++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                        dest[d_idx++] = (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: return RJSON_ERROR_PARSE_INVALID_STRING;
            }
        } else {
            dest[d_idx++] = c;
        }
    }
    
    dest[d_idx] = '\0';
    val->as.str_val.ptr = dest;
    val->as.str_val.len = d_idx;
    *out_value = val;
    return RJSON_OK;
}

static rjson_error_t parse_number(rjson_zone* zone, rjson_stream* stream, rjson_value** out_value) {
    size_t start = stream->position;
    
    if (rjson_stream_peek(stream) == '-') rjson_stream_get(stream);
    
    int c = rjson_stream_peek(stream);
    if (c == '0') {
        rjson_stream_get(stream);
        if (isdigit(rjson_stream_peek(stream))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    } else if (isdigit(c)) {
        while (isdigit(rjson_stream_peek(stream))) rjson_stream_get(stream);
    } else {
        return RJSON_ERROR_PARSE_INVALID_NUMBER;
    }
    
    if (rjson_stream_peek(stream) == '.') {
        rjson_stream_get(stream);
        if (!isdigit(rjson_stream_peek(stream))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
        while (isdigit(rjson_stream_peek(stream))) rjson_stream_get(stream);
    }
    
    c = rjson_stream_peek(stream);
    if (c == 'e' || c == 'E') {
        rjson_stream_get(stream);
        c = rjson_stream_peek(stream);
        if (c == '+' || c == '-') rjson_stream_get(stream);
        if (!isdigit(rjson_stream_peek(stream))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
        while (isdigit(rjson_stream_peek(stream))) rjson_stream_get(stream);
    }
    
    size_t len = stream->position - start;
    char num_buf[320]; 
    if (len >= sizeof(num_buf)) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    
    memcpy(num_buf, stream->data + start, len);
    num_buf[len] = '\0';
    
    char* end;
    double num = strtod(num_buf, &end);
    if (*end != '\0' || !isfinite(num)) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    
    *out_value = rjson_number_new(zone, num);
    if (!*out_value) return RJSON_ERROR_NOMEM;
    return RJSON_OK;
}

static rjson_error_t parse_literal(rjson_zone* zone, rjson_stream* stream, rjson_value** out_value) {
    const char* str = stream->data + stream->position;
    size_t rem = rjson_stream_remaining(stream);
    
    if (rem >= 4 && strncmp(str, "true", 4) == 0) {
        stream->position += 4;
        *out_value = rjson_bool_new(zone, 1);
        return *out_value ? RJSON_OK : RJSON_ERROR_NOMEM;
    }
    if (rem >= 5 && strncmp(str, "false", 5) == 0) {
        stream->position += 5;
        *out_value = rjson_bool_new(zone, 0);
        return *out_value ? RJSON_OK : RJSON_ERROR_NOMEM;
    }
    if (rem >= 4 && strncmp(str, "null", 4) == 0) {
        stream->position += 4;
        *out_value = rjson_null_new(zone);
        return *out_value ? RJSON_OK : RJSON_ERROR_NOMEM;
    }
    return RJSON_ERROR_PARSE_INVALID_FORMAT;
}

static rjson_error_t parse_array(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value** out_value) {
    if (depth >= RJSON_MAX_DEPTH) return RJSON_ERROR_PARSE_DEPTH_EXCEEDED;
    rjson_stream_get(stream); // skip '['
    
    rjson_value* arr = rjson_array_new(zone);
    if (!arr) return RJSON_ERROR_NOMEM;
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_peek(stream) == ']') {
        rjson_stream_get(stream);
        *out_value = arr;
        return RJSON_OK;
    }
    
    while (1) {
        rjson_value* element = NULL;
        rjson_error_t err = parse_value(zone, stream, depth + 1, &element);
        if (err != RJSON_OK) return err;
        
        if (rjson_array_add(zone, arr, element) != 0) return RJSON_ERROR_NOMEM;
        
        rjson_stream_skip_whitespace(stream);
        int c = rjson_stream_get(stream);
        if (c == ']') break;
        if (c != ',') return RJSON_ERROR_PARSE_INVALID_FORMAT;
    }
    
    *out_value = arr;
    return RJSON_OK;
}

static rjson_error_t parse_object(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value** out_value) {
    if (depth >= RJSON_MAX_DEPTH) return RJSON_ERROR_PARSE_DEPTH_EXCEEDED;
    rjson_stream_get(stream); // skip '{'
    
    rjson_value* obj = rjson_object_new(zone);
    if (!obj) return RJSON_ERROR_NOMEM;
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_peek(stream) == '}') {
        rjson_stream_get(stream);
        *out_value = obj;
        return RJSON_OK;
    }
    
    while (1) {
        rjson_stream_skip_whitespace(stream);
        if (rjson_stream_peek(stream) != '"') return RJSON_ERROR_PARSE_INVALID_FORMAT;
        
        rjson_value* key_val = NULL;
        rjson_error_t err = parse_string(zone, stream, &key_val);
        if (err != RJSON_OK) return err;
        
        rjson_stream_skip_whitespace(stream);
        if (rjson_stream_get(stream) != ':') return RJSON_ERROR_PARSE_INVALID_FORMAT;
        
        rjson_value* val = NULL;
        err = parse_value(zone, stream, depth + 1, &val);
        if (err != RJSON_OK) return err;
        
        if (rjson_object_add(zone, obj, key_val->as.str_val.ptr, key_val->as.str_val.len, val) != 0) return RJSON_ERROR_NOMEM;
        
        rjson_stream_skip_whitespace(stream);
        int c = rjson_stream_get(stream);
        if (c == '}') break;
        if (c != ',') return RJSON_ERROR_PARSE_INVALID_FORMAT;
    }
    
    *out_value = obj;
    return RJSON_OK;
}

static rjson_error_t parse_value(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value** out_value) {
    rjson_stream_skip_whitespace(stream);
    int c = rjson_stream_peek(stream);
    
    switch (c) {
        case '"': return parse_string(zone, stream, out_value);
        case '[': return parse_array(zone, stream, depth, out_value);
        case '{': return parse_object(zone, stream, depth, out_value);
        case 't':
        case 'f':
        case 'n': return parse_literal(zone, stream, out_value);
        default:
            if (c == '-' || isdigit(c)) return parse_number(zone, stream, out_value);
            if (c == -1) return RJSON_ERROR_PARSE_INCOMPLETE;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
    }
}

rjson_error_t rjson_parse_stream(rjson_zone* zone, rjson_stream* stream, rjson_value** out_value) {
    if (rjson_stream_remaining(stream) >= 3 && strncmp(stream->data + stream->position, "\xEF\xBB\xBF", 3) == 0) {
        stream->position += 3;
    }
    
    rjson_error_t err = parse_value(zone, stream, 0, out_value);
    if (err != RJSON_OK) return err;
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_remaining(stream) > 0) return RJSON_ERROR_PARSE_TRAILING_GARBAGE;
    
    return RJSON_OK;
}
