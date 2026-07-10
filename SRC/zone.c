#include "zone.h"
#include <stdlib.h>
#include <stdint.h>

#define RJSON_ZONE_PAGE_SIZE 4096

typedef struct rjson_zone_page {
    struct rjson_zone_page* next;
    size_t used;
    size_t capacity;
    uint8_t data[]; // Flexible array member
} rjson_zone_page;

struct rjson_zone {
    rjson_zone_page* head;
    rjson_zone_page* tail;
};

rjson_zone* rjson_zone_create(void) {
    rjson_zone* zone = (rjson_zone*)malloc(sizeof(rjson_zone));
    if (!zone) return NULL;
    zone->head = NULL;
    zone->tail = NULL;
    return zone;
}

static rjson_zone_page* alloc_page(size_t min_size) {
    size_t capacity = RJSON_ZONE_PAGE_SIZE;
    if (min_size > capacity) {
        capacity = min_size;
    }
    rjson_zone_page* page = (rjson_zone_page*)malloc(sizeof(rjson_zone_page) + capacity);
    if (!page) return NULL;
    page->next = NULL;
    page->used = 0;
    page->capacity = capacity;
    return page;
}

void* rjson_zone_alloc(rjson_zone* zone, size_t size) {
    if (!zone || size == 0) return NULL;
    
    // Ensure memory alignment (8 bytes)
    size = (size + 7) & ~7;
    
    if (!zone->tail) {
        zone->head = zone->tail = alloc_page(size);
        if (!zone->head) return NULL;
    } else if (zone->tail->used + size > zone->tail->capacity) {
        rjson_zone_page* new_page = alloc_page(size);
        if (!new_page) return NULL;
        zone->tail->next = new_page;
        zone->tail = new_page;
    }
    
    void* ptr = zone->tail->data + zone->tail->used;
    zone->tail->used += size;
    return ptr;
}

void rjson_zone_destroy(rjson_zone* zone) {
    if (!zone) return;
    rjson_zone_page* current = zone->head;
    while (current) {
        rjson_zone_page* next = current->next;
        free(current);
        current = next;
    }
    free(zone);
}
