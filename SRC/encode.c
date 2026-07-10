#include "encode.h"
#include "rjson.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "tools/tables.h"
#include "tools/endian.h"
#include "tools/compiler.h"

#define RJSON_MAX_DEPTH 512

// Fast integer to string (returns length)
static inline int format_uint64(uint64_t v, char* buf) {
    if (v == 0) {
        buf[0] = '0';
        return 1;
    }
    char temp[32];
    int idx = 31;
    while (v > 0) {
        temp[--idx] = '0' + (v % 10);
        v /= 10;
    }
    int len = 31 - idx;
    memcpy(buf, temp + idx, len);
    return len;
}

// Very basic and fast double-to-string for numbers in [-1e10, 1e10] with up to 6 decimal places
// This covers 99% of normal JSON payloads (e.g. coordinates, prices, metrics).
static inline int fast_dtoa(double n, char* buf) {
    if (n != n) { memcpy(buf, "NaN", 3); return 3; }
    if (n < -1e10 || n > 1e10) return 0; // fallback to snprintf
    
    int idx = 0;
    if (n < 0) {
        buf[idx++] = '-';
        n = -n;
    }
    
    uint64_t int_part = (uint64_t)n;
    double frac_part = n - (double)int_part;
    
    // Add integer part
    idx += format_uint64(int_part, buf + idx);
    
    // Check if there is a fractional part
    if (frac_part > 1e-8 && frac_part < 0.99999999) {
        buf[idx++] = '.';
        
        // Multiply by 1,000,000 to get 6 decimal places
        uint32_t frac = (uint32_t)(frac_part * 1000000.0 + 0.5);
        
        // Handle rounding overflow (e.g. 0.9999996 -> 1.0)
        if (frac >= 1000000) {
            return 0; // fallback just to be safe
        }
        
        // Remove trailing zeros
        int trailing_zeros = 0;
        while (frac > 0 && (frac % 10) == 0) {
            frac /= 10;
            trailing_zeros++;
        }
        
        if (frac == 0) {
            idx--; // remove '.'
            return idx;
        }
        
        int digits = 6 - trailing_zeros;
        char temp[8];
        for (int i = digits - 1; i >= 0; i--) {
            temp[i] = '0' + (frac % 10);
            frac /= 10;
        }
        memcpy(buf + idx, temp, digits);
        idx += digits;
    }
    return idx;
}

#define RJSON_OUT_PUTC(stream, ch) do { \
    if ((stream)->length + 2 > (stream)->capacity) { \
        rjson_out_stream_grow((stream), (stream)->length + 2); \
    } \
    (stream)->buffer[(stream)->length++] = (ch); \
} while(0)

#define RJSON_OUT_PUT4(stream, val) do { \
    if ((stream)->length + 4 > (stream)->capacity) { \
        rjson_out_stream_grow((stream), (stream)->length + 4); \
    } \
    serialize_be32((stream)->buffer + (stream)->length, (val)); \
    (stream)->length += 4; \
} while(0)

#define RJSON_OUT_PUT5(stream, val4, val1) do { \
    if ((stream)->length + 5 > (stream)->capacity) { \
        rjson_out_stream_grow((stream), (stream)->length + 5); \
    } \
    serialize_be32((stream)->buffer + (stream)->length, (val4)); \
    (stream)->buffer[(stream)->length + 4] = (val1); \
    (stream)->length += 5; \
} while(0)

#define RJSON_RAW_PUT4(stream, val) do { \
    serialize_be32((stream)->buffer + (stream)->length, (val)); \
    (stream)->length += 4; \
} while(0)

#define RJSON_RAW_PUT5(stream, val4, val1) do { \
    serialize_be32((stream)->buffer + (stream)->length, (val4)); \
    (stream)->buffer[(stream)->length + 4] = (val1); \
    (stream)->length += 5; \
} while(0)

#define RJSON_OUT_PUT2(stream, val) do { \
    if ((stream)->length + 2 > (stream)->capacity) { \
        rjson_out_stream_grow((stream), (stream)->length + 2); \
    } \
    serialize_be16((stream)->buffer + (stream)->length, (val)); \
    (stream)->length += 2; \
} while(0)

#define RJSON_RAW_PUTC(stream, ch) do { \
    (stream)->buffer[(stream)->length++] = (ch); \
} while(0)

#define RJSON_RAW_PUT2(stream, val) do { \
    serialize_be16((stream)->buffer + (stream)->length, (val)); \
    (stream)->length += 2; \
} while(0)

#define SWAR_HAS_ZERO(v) (((v) - 0x0101010101010101ULL) & ~(v) & 0x8080808080808080ULL)
#define SWAR_HAS_CHAR(v, c) SWAR_HAS_ZERO((v) ^ (~0ULL / 255 * (c)))

static const uint16_t escape_table[256] = {
    ['"'] = 0x5c22,
    ['\\'] = 0x5c5c,
    ['\b'] = 0x5c62,
    ['\f'] = 0x5c66,
    ['\n'] = 0x5c6e,
    ['\r'] = 0x5c72,
    ['\t'] = 0x5c74
};

static rjson_error_t encode_value(rjson_out_stream* stream, const rjson_value* value, int depth);

static rjson_error_t encode_string(rjson_out_stream* stream, const rjson_value* str_val) {
    size_t len = rjson_get_len(str_val);
    const char* ptr = str_val->as.str_val;
    size_t max_needed = len * 6 + 2;
    if (rjson_unlikely(stream->length + max_needed > stream->capacity)) {
        if (rjson_out_stream_grow(stream, stream->length + max_needed) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    }
    
    RJSON_RAW_PUTC(stream, '"');
    
    size_t last_flushed = 0;
    size_t i = 0;
    while (i < len) {
        while (i + 7 < len) {
            uint64_t v;
            memcpy(&v, ptr + i, 8);
            if (SWAR_HAS_CHAR(v, '"') || SWAR_HAS_CHAR(v, '\\') || SWAR_HAS_ZERO(v & 0xE0E0E0E0E0E0E0E0ULL)) {
                break;
            }
            i += 8;
        }
        
        while (i < len) {
            unsigned char c = (unsigned char)ptr[i];
            if (rjson_unlikely(c < 0x20 || c == '"' || c == '\\')) {
                if (rjson_likely(i > last_flushed)) {
                    memcpy(stream->buffer + stream->length, ptr + last_flushed, i - last_flushed);
                    stream->length += (i - last_flushed);
                }
                uint16_t esc = escape_table[c];
                if (rjson_likely(esc)) {
                    RJSON_RAW_PUT2(stream, esc);
                } else {
                    char buf[6] = {'\\', 'u', '0', '0', "0123456789abcdef"[(c >> 4) & 0xF], "0123456789abcdef"[c & 0xF]};
                    memcpy(stream->buffer + stream->length, buf, 6);
                    stream->length += 6;
                }
                last_flushed = i + 1;
                i++;
                break;
            }
            i++;
        }
    }
    
    if (len > last_flushed) {
        memcpy(stream->buffer + stream->length, ptr + last_flushed, len - last_flushed);
        stream->length += (len - last_flushed);
    }
    
    RJSON_RAW_PUTC(stream, '"');
    return RJSON_OK;
}

static rjson_error_t encode_array(rjson_out_stream* stream, const rjson_value* arr, int depth) {
    if (rjson_unlikely(depth >= RJSON_MAX_DEPTH)) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    RJSON_OUT_PUTC(stream, '[');
    
    size_t count = rjson_get_len(arr);
    rjson_value* elements = arr->as.elements;
    for (size_t i = 0; i < count; i++) {
        rjson_error_t err = encode_value(stream, &elements[i], depth + 1);
        if (rjson_unlikely(err != RJSON_OK)) return err;
        
        if (i < count - 1) {
            RJSON_OUT_PUTC(stream, ',');
        }
    }
    
    RJSON_OUT_PUTC(stream, ']');
    return RJSON_OK;
}

static rjson_error_t encode_object(rjson_out_stream* stream, const rjson_value* obj, int depth) {
    if (rjson_unlikely(depth >= RJSON_MAX_DEPTH)) return RJSON_ERROR_SERIALIZE_DEPTH_EXCEEDED;
    RJSON_OUT_PUTC(stream, '{');
    
    size_t count = rjson_get_len(obj);
    rjson_kv* kvs = obj->as.kvs;
    for (size_t i = 0; i < count; i++) {
        rjson_error_t err = encode_string(stream, &kvs[i].key);
        if (rjson_unlikely(err != RJSON_OK)) return err;
        
        RJSON_OUT_PUTC(stream, ':');
        
        err = encode_value(stream, &kvs[i].value, depth + 1);
        if (rjson_unlikely(err != RJSON_OK)) return err;
        
        if (i < count - 1) {
            RJSON_OUT_PUTC(stream, ',');
        }
    }
    
    RJSON_OUT_PUTC(stream, '}');
    return RJSON_OK;
}

static rjson_error_t encode_value(rjson_out_stream* stream, const rjson_value* value, int depth) {
    if (rjson_unlikely(!value)) return RJSON_ERROR_OBJECT_NULL_POINTER;
    
    if (rjson_unlikely(stream->length + 64 > stream->capacity)) {
        if (rjson_out_stream_grow(stream, stream->length + 64) != 0) return RJSON_ERROR_SERIALIZE_NOMEM;
    }
    
    switch (rjson_get_type(value)) {
        case RJSON_NULL:
            RJSON_RAW_PUT4(stream, 0x6e756c6c); // 'null'
            return RJSON_OK;
        case RJSON_BOOL:
            if (value->as.bool_val) {
                RJSON_RAW_PUT4(stream, 0x74727565); // 'true'
            } else {
                RJSON_RAW_PUT5(stream, 0x66616c73, 'e'); // 'fals' + 'e'
            }
            return RJSON_OK;
        case RJSON_NUMBER: {
            double n = value->as.num_val;
            if (floor(n) == n && n >= -9007199254740992.0 && n <= 9007199254740992.0) {
                int64_t v = (int64_t)n;
                if (v == 0) {
                    RJSON_RAW_PUTC(stream, '0');
                    return RJSON_OK;
                }
                char temp[32];
                int idx = 31;
                int is_neg = (v < 0);
                if (is_neg) v = -v;
                while (v > 0) {
                    temp[--idx] = '0' + (v % 10);
                    v /= 10;
                }
                if (is_neg) temp[--idx] = '-';
                int len = 31 - idx;
                memcpy(stream->buffer + stream->length, temp + idx, len);
                stream->length += len;
                return RJSON_OK;
            } else if (floor(n * 10.0) == n * 10.0 && (n * 10.0) >= -9007199254740992.0 && (n * 10.0) <= 9007199254740992.0) {
                int64_t v = (int64_t)(n * 10.0);
                if (v == 0) {
                    RJSON_RAW_PUTC(stream, '0');
                    RJSON_RAW_PUTC(stream, '.');
                    RJSON_RAW_PUTC(stream, '0');
                    return RJSON_OK;
                }
                char temp[32];
                int idx = 31;
                int is_neg = (v < 0);
                if (is_neg) v = -v;
                
                temp[--idx] = '0' + (v % 10);
                v /= 10;
                temp[--idx] = '.';
                
                if (v == 0) {
                    temp[--idx] = '0';
                } else {
                    while (v > 0) {
                        temp[--idx] = '0' + (v % 10);
                        v /= 10;
                    }
                }
                if (is_neg) temp[--idx] = '-';
                int len = 31 - idx;
                memcpy(stream->buffer + stream->length, temp + idx, len);
                stream->length += len;
                return RJSON_OK;
            } else {
                int len = fast_dtoa(n, stream->buffer + stream->length);
                if (len == 0) {
                    len = snprintf(stream->buffer + stream->length, 64, "%.17g", n);
                }
                stream->length += len;
                return RJSON_OK;
            }

        }
        case RJSON_STRING:
            return encode_string(stream, value);
        case RJSON_ARRAY:
            return encode_array(stream, value, depth);
        case RJSON_OBJECT:
            return encode_object(stream, value, depth);
        default:
            return RJSON_ERROR_OBJECT_TYPE_MISMATCH;
    }
}

rjson_error_t rjson_encode_stream(rjson_out_stream* stream, const rjson_value* value) {
    if (!stream || !value) return RJSON_ERROR_BAD_ARG;
    return encode_value(stream, value, 0);
}
