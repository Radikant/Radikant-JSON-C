#include "decode.h"
#include "rjson.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "tools/tables.h"
#include "tools/endian.h"
#include "tools/compiler.h"
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define RJSON_MAX_DEPTH 512

static rjson_error_t decode_value(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value* out_value);
static rjson_error_t decode_raw_string(rjson_zone* zone, rjson_stream* stream, const char** out_str, size_t* out_len);

static int decode_hex4(const char* p, uint32_t* out_cp) {
    uint32_t c0 = hex_conv_table[(unsigned char)p[0]];
    uint32_t c1 = hex_conv_table[(unsigned char)p[1]];
    uint32_t c2 = hex_conv_table[(unsigned char)p[2]];
    uint32_t c3 = hex_conv_table[(unsigned char)p[3]];
    
    if ((c0 | c1 | c2 | c3) & 0xF0) return -1;
    
    *out_cp = (c0 << 12) | (c1 << 8) | (c2 << 4) | c3;
    return 0;
}

static const unsigned char rjson_char_is_stop[256] = {
    // 0x00 to 0x1F are control chars (invalid unescaped in JSON)
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    ['"'] = 2,
    ['\\'] = 3
};

static rjson_error_t decode_raw_string(rjson_zone* zone, rjson_stream* stream, const char** out_str, size_t* out_len) {
    stream->position++; // skip '"'
    
    size_t start_pos = stream->position;
    int has_escapes = 0;
    
    const char* p = stream->data + stream->position;
    const char* end = stream->data + stream->length;
    
#define SWAR_HAS_ZERO(v) (((v) - 0x0101010101010101ULL) & ~(v) & 0x8080808080808080ULL)
#define SWAR_HAS_CHAR(v, c) SWAR_HAS_ZERO((v) ^ ((0x0101010101010101ULL) * (c)))

    // Fast scan SWAR (8 bytes at a time)
    while (rjson_likely(p + 7 < end)) {
        uint64_t v;
        memcpy(&v, p, 8);
        if (rjson_unlikely(SWAR_HAS_CHAR(v, '"') || SWAR_HAS_CHAR(v, '\\') || SWAR_HAS_ZERO(v & 0xE0E0E0E0E0E0E0E0ULL))) {
            break;
        }
        p += 8;
    }
    
    // Fast scan fallback (byte by byte)
    while (rjson_likely(p < end)) {
        unsigned char stop_type = rjson_char_is_stop[(unsigned char)*p];
        if (rjson_unlikely(stop_type)) {
            if (stop_type == 2) break; // '"'
            if (stop_type == 3) {
                has_escapes = 1;
                p++; // skip the backslash
                if (p >= end) return RJSON_ERROR_PARSE_INCOMPLETE;
            } else {
                return RJSON_ERROR_PARSE_INVALID_STRING; // control char
            }
        }
        p++;
    }
    
    if (rjson_unlikely(p >= end)) return RJSON_ERROR_PARSE_INCOMPLETE;
    
    size_t string_len = p - (stream->data + start_pos);
    const char* string_start = stream->data + start_pos;
    
    stream->position = (p - stream->data) + 1; // skip closing '"'
    
    if (!has_escapes) {
        // Zero-copy fast path!
        *out_str = string_start;
        *out_len = string_len;
        return RJSON_OK;
    }
    
    // Slow path: allocate from zone and unescape
    char* dest = (char*)rjson_zone_alloc(zone, string_len + 1);
    if (rjson_unlikely(!dest)) return RJSON_ERROR_NOMEM;
    
    size_t d_idx = 0;
    size_t i = 0;
    size_t last_copied = 0;
    
    while (i < string_len) {
        char c = string_start[i];
        if (c == '\\') {
            if (i > last_copied) {
                memcpy(dest + d_idx, string_start + last_copied, i - last_copied);
                d_idx += (i - last_copied);
            }
            i++;
            if (rjson_unlikely(i >= string_len)) return RJSON_ERROR_PARSE_INVALID_STRING;
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
                    if (rjson_unlikely(i + 4 >= string_len)) return RJSON_ERROR_PARSE_INVALID_STRING;
                    uint32_t cp;
                    if (rjson_unlikely(decode_hex4(string_start + i + 1, &cp) != 0)) return RJSON_ERROR_PARSE_INVALID_STRING;
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
                    if (rjson_unlikely(cp >= 0xD800 && cp <= 0xDFFF)) return RJSON_ERROR_PARSE_INVALID_STRING; 
                    
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
            i++;
            last_copied = i;
        } else {
            i++;
        }
    }
    
    if (i > last_copied) {
        memcpy(dest + d_idx, string_start + last_copied, i - last_copied);
        d_idx += (i - last_copied);
    }
    
    dest[d_idx] = '\0';
    *out_str = dest;
    *out_len = d_idx;
    return RJSON_OK;
}


static rjson_error_t decode_string(rjson_zone* zone, rjson_stream* stream, rjson_value* out_value) {
    const char* str = NULL;
    size_t len = 0;
    rjson_error_t err = decode_raw_string(zone, stream, &str, &len);
    if (err == RJSON_OK) {
        rjson_string_init(out_value, str, len);
    }
    return err;
}

static inline void skip_whitespace(rjson_stream* stream) {
    const char* p = stream->data + stream->position;
    const char* end = stream->data + stream->length;
    while (p < end && (char_table1[(unsigned char)*p] & CHAR_TYPE_SPACE)) {
        p++;
    }
    stream->position = p - stream->data;
}

static rjson_error_t decode_number(rjson_zone* zone, rjson_stream* stream, rjson_value* out_value) {
    if (rjson_unlikely(!stream || !out_value)) return RJSON_ERROR_BAD_ARG;
    if (rjson_unlikely(stream->position >= stream->length)) return RJSON_ERROR_PARSE_INCOMPLETE;
    
    size_t start = stream->position;
    const char* p = stream->data + stream->position;
    const char* end = stream->data + stream->length;
    
    int is_negative = 0;
    if (p < end && *p == '-') {
        is_negative = 1;
        p++;
    }
    
    if (rjson_unlikely(p >= end || ((unsigned)(*p - '0') > 9))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    
    uint64_t mantissa = 0;
    int exp = 0;
    int digit_count = 0;
    int is_float = 0;
    
    if (*p == '0') {
        p++;
        digit_count = 1;
        if (p < end && (char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT)) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    } else {
        while (p < end && (char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT)) {
            if (digit_count < 19) {
                mantissa = mantissa * 10 + (*p - '0');
            } else {
                exp++;
            }
            digit_count++;
            p++;
        }
    }
    
    if (p < end && *p == '.') {
        is_float = 1;
        p++;
        if (p >= end || !(char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT)) return RJSON_ERROR_PARSE_INVALID_NUMBER;
        while (p < end && (char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT)) {
            if (digit_count < 19) {
                mantissa = mantissa * 10 + (*p - '0');
                exp--;
                digit_count++;
            }
            p++;
        }
    }
    
    if (p < end && (*p == 'e' || *p == 'E')) {
        is_float = 1;
        p++;
        int exp_neg = 0;
        if (p < end && *p == '+') {
            p++;
        } else if (p < end && *p == '-') {
            exp_neg = 1;
            p++;
        }
        if (rjson_unlikely(p >= end || !(char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
        int explicit_exp = 0;
        while (p < end && (char_table3[(unsigned char)*p] & CHAR_TYPE_DIGIT)) {
            if (explicit_exp < 10000) explicit_exp = explicit_exp * 10 + (*p - '0');
            p++;
        }
        exp += (exp_neg ? -explicit_exp : explicit_exp);
    }
    
    stream->position = p - stream->data;
    
    double num;
    if (!is_float && digit_count <= 19) {
        num = (double)mantissa;
        if (is_negative) num = -num;
    } else if (exp >= -22 && exp <= 22 && digit_count <= 19) {
        if (exp < 0) {
            num = (double)mantissa / f64_pow10_table[-exp];
        } else {
            num = (double)mantissa * f64_pow10_table[exp];
        }
        if (is_negative) num = -num;
    } else {
        // Slow path: strtod
        size_t len = stream->position - start;
        char num_buf[320]; 
        if (rjson_unlikely(len >= sizeof(num_buf))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
        
        memcpy(num_buf, stream->data + start, len);
        num_buf[len] = '\0';
        
        char* endptr;
        num = strtod(num_buf, &endptr);
        if (rjson_unlikely(*endptr != '\0' || !isfinite(num))) return RJSON_ERROR_PARSE_INVALID_NUMBER;
    }
    
    rjson_number_init(out_value, num);
    return RJSON_OK;
}

static rjson_error_t decode_literal(rjson_zone* zone, rjson_stream* stream, rjson_value* out_value) {
    const char* str = stream->data + stream->position;
    size_t rem = rjson_stream_remaining(stream);
    
    if (rem >= 4) {
        uint32_t val = deserialize_be32(str);
        if (val == 0x74727565) { // 'true'
            stream->position += 4;
            rjson_bool_init(out_value, 1);
            return RJSON_OK;
        }
        if (val == 0x6E756C6C) { // 'null'
            stream->position += 4;
            rjson_null_init(out_value);
            return RJSON_OK;
        }
        if (rem >= 5) {
            if (val == 0x66616C73 && str[4] == 'e') { // 'fals' + 'e'
                stream->position += 5;
                rjson_bool_init(out_value, 0);
                return RJSON_OK;
            }
        }
    }
    return RJSON_ERROR_PARSE_INVALID_FORMAT;
}

static rjson_error_t decode_array(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value* out_value) {
    if (rjson_unlikely(depth >= RJSON_MAX_DEPTH)) return RJSON_ERROR_PARSE_DEPTH_EXCEEDED;
    rjson_stream_get(stream); // skip '['
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_peek(stream) == ']') {
        rjson_stream_get(stream);
        rjson_array_init(out_value);
        return RJSON_OK;
    }
    
    size_t stack_start = stream->val_stack_count;
    
    while (1) {
        rjson_value element;
        rjson_error_t err = decode_value(zone, stream, depth + 1, &element);
        if (rjson_unlikely(err != RJSON_OK)) {
            stream->val_stack_count = stack_start;
            return err;
        }
        if (rjson_unlikely(rjson_stream_push_val(stream, element) != 0)) return RJSON_ERROR_NOMEM;
        
        rjson_stream_skip_whitespace(stream);
        int c = rjson_stream_get(stream);
        if (c == ']') break;
        if (rjson_unlikely(c != ',')) {
            stream->val_stack_count = stack_start;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
        }
    }
    
    size_t count = stream->val_stack_count - stack_start;
    rjson_value* elements = (rjson_value*)rjson_zone_alloc(zone, count * sizeof(rjson_value));
    if (rjson_unlikely(!elements)) {
        stream->val_stack_count = stack_start;
        return RJSON_ERROR_NOMEM;
    }
    
    memcpy(elements, stream->val_stack + stack_start, count * sizeof(rjson_value));
    stream->val_stack_count = stack_start;
    
    rjson_set_tag(out_value, RJSON_ARRAY, count);
    out_value->as.elements = elements;
    
    return RJSON_OK;
}

static rjson_error_t decode_object(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value* out_value) {
    if (rjson_unlikely(depth >= RJSON_MAX_DEPTH)) return RJSON_ERROR_PARSE_DEPTH_EXCEEDED;
    rjson_stream_get(stream); // skip '{'
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_peek(stream) == '}') {
        rjson_stream_get(stream);
        rjson_object_init(out_value);
        return RJSON_OK;
    }
    
    size_t stack_start = stream->kv_stack_count;
    
    while (1) {
        rjson_stream_skip_whitespace(stream);
        if (rjson_unlikely(rjson_stream_peek(stream) != '"')) {
            stream->kv_stack_count = stack_start;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
        }
        
        rjson_kv kv;
        const char* key_ptr;
        size_t key_len;
        rjson_error_t err = decode_raw_string(zone, stream, &key_ptr, &key_len);
        if (rjson_unlikely(err != RJSON_OK)) {
            stream->kv_stack_count = stack_start;
            return err;
        }
        rjson_set_tag(&kv.key, RJSON_STRING, key_len);
        kv.key.as.str_val = key_ptr;
        
        rjson_stream_skip_whitespace(stream);
        if (rjson_unlikely(rjson_stream_get(stream) != ':')) {
            stream->kv_stack_count = stack_start;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
        }
        
        err = decode_value(zone, stream, depth + 1, &kv.value);
        if (rjson_unlikely(err != RJSON_OK)) {
            stream->kv_stack_count = stack_start;
            return err;
        }
        
        if (rjson_unlikely(rjson_stream_push_kv(stream, kv) != 0)) return RJSON_ERROR_NOMEM;
        
        rjson_stream_skip_whitespace(stream);
        int c = rjson_stream_get(stream);
        if (c == '}') break;
        if (rjson_unlikely(c != ',')) {
            stream->kv_stack_count = stack_start;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
        }
    }
    
    size_t count = stream->kv_stack_count - stack_start;
    rjson_kv* kvs = (rjson_kv*)rjson_zone_alloc(zone, count * sizeof(rjson_kv));
    if (rjson_unlikely(!kvs)) {
        stream->kv_stack_count = stack_start;
        return RJSON_ERROR_NOMEM;
    }
    
    memcpy(kvs, stream->kv_stack + stack_start, count * sizeof(rjson_kv));
    stream->kv_stack_count = stack_start;
    
    rjson_set_tag(out_value, RJSON_OBJECT, count);
    out_value->as.kvs = kvs;
    
    return RJSON_OK;
}

static rjson_error_t decode_value(rjson_zone* zone, rjson_stream* stream, int depth, rjson_value* out_value) {
    rjson_stream_skip_whitespace(stream);
    int c = rjson_stream_peek(stream);
    
    switch (c) {
        case '"': return decode_string(zone, stream, out_value);
        case '[': return decode_array(zone, stream, depth, out_value);
        case '{': return decode_object(zone, stream, depth, out_value);
        case 't':
        case 'f':
        case 'n': return decode_literal(zone, stream, out_value);
        default:
            if (c == '-' || ((unsigned)(c - '0') <= 9)) return decode_number(zone, stream, out_value);
            if (c == -1) return RJSON_ERROR_PARSE_INCOMPLETE;
            return RJSON_ERROR_PARSE_INVALID_FORMAT;
    }
}

rjson_error_t rjson_decode_stream(rjson_zone* zone, rjson_stream* stream, rjson_value* out_value) {
    if (rjson_stream_remaining(stream) >= 3 && strncmp(stream->data + stream->position, "\xEF\xBB\xBF", 3) == 0) {
        stream->position += 3;
    }
    
    rjson_error_t err = decode_value(zone, stream, 0, out_value);
    if (err != RJSON_OK) return err;
    
    rjson_stream_skip_whitespace(stream);
    if (rjson_stream_remaining(stream) > 0) return RJSON_ERROR_PARSE_TRAILING_GARBAGE;
    
    return RJSON_OK;
}