#ifndef RJSON_ZONE_H
#define RJSON_ZONE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#define RJSON_ZONE_PAGE_SIZE 4096

typedef struct rjson_zone_page {
    struct rjson_zone_page* next;
    size_t used;
    size_t capacity;
    uint64_t _padding; // align data to 32-bytes for cache lines
    uint8_t data[]; // Flexible array member
} rjson_zone_page;

typedef struct rjson_zone {
    rjson_zone_page* head;
    rjson_zone_page* tail;
    size_t next_page_size;
} rjson_zone;

/**
 * @brief Creates a new memory arena (zone).
 */
rjson_zone* rjson_zone_create(void);

/**
 * @brief Allocates a new page for the zone (slow path).
 */
void* rjson_zone_alloc_slow(rjson_zone* zone, size_t size);

/**
 * @brief Allocates memory from the zone (fast inline path).
 * Memory allocated this way is guaranteed to be 8-byte aligned.
 */
static inline void* rjson_zone_alloc(rjson_zone* zone, size_t size) {
    if (!zone || size == 0) return NULL;
    
    // Ensure memory alignment (8 bytes)
    size = (size + 7) & ~7;
    
    rjson_zone_page* tail = zone->tail;
    if (tail && tail->used + size <= tail->capacity) {
        void* ptr = tail->data + tail->used;
        tail->used += size;
        return ptr;
    }
    
    return rjson_zone_alloc_slow(zone, size);
}

/**
 * @brief Reallocates memory in the zone (slow path).
 */
void* rjson_zone_realloc_slow(rjson_zone* zone, void* ptr, size_t old_size, size_t new_size);

/**
 * @brief Reallocates memory in the zone (fast inline path).
 * If the ptr is the last allocation and there is space in the page, it expands in-place.
 */
static inline void* rjson_zone_realloc(rjson_zone* zone, void* ptr, size_t old_size, size_t new_size) {
    if (!zone || new_size == 0) return NULL;
    if (!ptr) return rjson_zone_alloc(zone, new_size);
    
    size_t aligned_old = (old_size + 7) & ~7;
    size_t aligned_new = (new_size + 7) & ~7;
    
    if (aligned_new <= aligned_old) return ptr;
    
    rjson_zone_page* tail = zone->tail;
    if (tail) {
        uint8_t* u_ptr = (uint8_t*)ptr;
        // Check if ptr is the last allocation in this page
        if (u_ptr + aligned_old == tail->data + tail->used) {
            size_t diff = aligned_new - aligned_old;
            if (tail->used + diff <= tail->capacity) {
                tail->used += diff;
                return ptr;
            }
        }
    }
    
    return rjson_zone_realloc_slow(zone, ptr, old_size, new_size);
}

/**
 * @brief Destroys the zone and frees all memory allocated within it.
 */
void rjson_zone_destroy(rjson_zone* zone);

#ifdef __cplusplus
}
#endif

#endif // RJSON_ZONE_H
