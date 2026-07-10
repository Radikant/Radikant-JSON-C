#include "zone.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


rjson_zone* rjson_zone_create(void) {
    rjson_zone* zone = (rjson_zone*)malloc(sizeof(rjson_zone));
    if (!zone) return NULL;
    zone->head = NULL;
    zone->tail = NULL;
    zone->next_page_size = RJSON_ZONE_PAGE_SIZE;
    return zone;
}

static rjson_zone_page* alloc_page(size_t capacity) {
    rjson_zone_page* page = (rjson_zone_page*)malloc(sizeof(rjson_zone_page) + capacity);
    if (!page) return NULL;
    page->next = NULL;
    page->used = 0;
    page->capacity = capacity;
    return page;
}

void* rjson_zone_alloc_slow(rjson_zone* zone, size_t size) {
    size_t alloc_cap = zone->next_page_size;
    if (size > alloc_cap) {
        alloc_cap = size;
    }
    
    rjson_zone_page* new_page = alloc_page(alloc_cap);
    if (!new_page) return NULL;
    
    if (!zone->tail) {
        zone->head = zone->tail = new_page;
    } else {
        zone->tail->next = new_page;
        zone->tail = new_page;
    }
    
    // Exponential growth, max 1MB per page
    if (zone->next_page_size < 1048576) {
        zone->next_page_size *= 2;
    }
    
    void* ptr = zone->tail->data + zone->tail->used;
    zone->tail->used += size;
    return ptr;
}

void* rjson_zone_realloc_slow(rjson_zone* zone, void* ptr, size_t old_size, size_t new_size) {
    void* new_ptr = rjson_zone_alloc(zone, new_size);
    if (!new_ptr) return NULL;
    
    if (ptr && old_size > 0) {
        size_t copy_size = old_size < new_size ? old_size : new_size;
        memcpy(new_ptr, ptr, copy_size);
    }
    return new_ptr;
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