#ifndef RJSON_ZONE_H
#define RJSON_ZONE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rjson_zone rjson_zone;

/**
 * @brief Creates a new memory arena (zone).
 */
rjson_zone* rjson_zone_create(void);

/**
 * @brief Allocates memory from the zone.
 * Memory allocated this way is guaranteed to be 8-byte aligned.
 */
void* rjson_zone_alloc(rjson_zone* zone, size_t size);

/**
 * @brief Destroys the zone and frees all memory allocated within it.
 */
void rjson_zone_destroy(rjson_zone* zone);

#ifdef __cplusplus
}
#endif

#endif // RJSON_ZONE_H
