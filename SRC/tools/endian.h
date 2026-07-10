#ifndef RADIKANT_JSON_TOOLS_ENDIAN_H
#define RADIKANT_JSON_TOOLS_ENDIAN_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Detect endianness and compiler support
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) &&             \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define RADIKANT_LITTLE_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                      \
    defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) ||      \
    defined(__MIPSEL) || defined(__MIPSEL__)
#define RADIKANT_LITTLE_ENDIAN 1
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86) ||             \
                            defined(_M_ARM64) || defined(_M_ARM))
#define RADIKANT_LITTLE_ENDIAN 1
#else
#define RADIKANT_LITTLE_ENDIAN 0
#endif

// Determine if we can use built-in bswap
#if RADIKANT_LITTLE_ENDIAN && (defined(__GNUC__) || defined(__clang__))
#define RADIKANT_USE_BUILTIN_BSWAP 1
#define RADIKANT_BSWAP16(x) __builtin_bswap16(x)
#define RADIKANT_BSWAP32(x) __builtin_bswap32(x)
#define RADIKANT_BSWAP64(x) __builtin_bswap64(x)
#elif RADIKANT_LITTLE_ENDIAN && defined(_MSC_VER)
#include <stdlib.h>
#define RADIKANT_USE_BUILTIN_BSWAP 1
#define RADIKANT_BSWAP16(x) _byteswap_ushort(x)
#define RADIKANT_BSWAP32(x) _byteswap_ulong(x)
#define RADIKANT_BSWAP64(x) _byteswap_uint64(x)
#else
#define RADIKANT_USE_BUILTIN_BSWAP 0
#endif

static inline uint16_t deserialize_be16(const char *buf) {
#if RADIKANT_USE_BUILTIN_BSWAP
  uint16_t val;
  memcpy(&val, buf, sizeof(val));
  return RADIKANT_BSWAP16(val);
#else
  return ((uint16_t)(uint8_t)buf[0] << 8) | (uint16_t)(uint8_t)buf[1];
#endif
}

static inline uint32_t deserialize_be32(const char *buf) {
#if RADIKANT_USE_BUILTIN_BSWAP
  uint32_t val;
  memcpy(&val, buf, sizeof(val));
  return RADIKANT_BSWAP32(val);
#else
  return ((uint32_t)(uint8_t)buf[0] << 24) | ((uint32_t)(uint8_t)buf[1] << 16) |
         ((uint32_t)(uint8_t)buf[2] << 8) | (uint32_t)(uint8_t)buf[3];
#endif
}

static inline uint64_t deserialize_be64(const char *buf) {
#if RADIKANT_USE_BUILTIN_BSWAP
  uint64_t val;
  memcpy(&val, buf, sizeof(val));
  return RADIKANT_BSWAP64(val);
#else
  return ((uint64_t)(uint8_t)buf[0] << 56) | ((uint64_t)(uint8_t)buf[1] << 48) |
         ((uint64_t)(uint8_t)buf[2] << 40) | ((uint64_t)(uint8_t)buf[3] << 32) |
         ((uint64_t)(uint8_t)buf[4] << 24) | ((uint64_t)(uint8_t)buf[5] << 16) |
         ((uint64_t)(uint8_t)buf[6] << 8) | (uint64_t)(uint8_t)buf[7];
#endif
}

static inline void serialize_be16(char *buf, uint16_t v) {
#if RADIKANT_USE_BUILTIN_BSWAP
  v = RADIKANT_BSWAP16(v);
  memcpy(buf, &v, sizeof(v));
#else
  buf[0] = (char)(v >> 8);
  buf[1] = (char)v;
#endif
}

static inline void serialize_be32(char *buf, uint32_t v) {
#if RADIKANT_USE_BUILTIN_BSWAP
  v = RADIKANT_BSWAP32(v);
  memcpy(buf, &v, sizeof(v));
#else
  buf[0] = (char)(v >> 24);
  buf[1] = (char)(v >> 16);
  buf[2] = (char)(v >> 8);
  buf[3] = (char)v;
#endif
}

static inline void serialize_be64(char *buf, uint64_t v) {
#if RADIKANT_USE_BUILTIN_BSWAP
  v = RADIKANT_BSWAP64(v);
  memcpy(buf, &v, sizeof(v));
#else
  buf[0] = (char)(v >> 56);
  buf[1] = (char)(v >> 48);
  buf[2] = (char)(v >> 40);
  buf[3] = (char)(v >> 32);
  buf[4] = (char)(v >> 24);
  buf[5] = (char)(v >> 16);
  buf[6] = (char)(v >> 8);
  buf[7] = (char)v;
#endif
}

#ifdef __cplusplus
}
#endif

#endif // RADIKANT_MESSAGEPACK_ENDIAN_H