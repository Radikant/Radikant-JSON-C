#ifndef RADIKANT_JSON_TOOLS_COMPILER_H
#define RADIKANT_JSON_TOOLS_COMPILER_H

#if defined(__GNUC__) || defined(__clang__)
    #define rjson_likely(x)   __builtin_expect(!!(x), 1)
    #define rjson_unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define rjson_likely(x)   (x)
    #define rjson_unlikely(x) (x)
#endif

#endif // RADIKANT_JSON_TOOLS_COMPILER_H
