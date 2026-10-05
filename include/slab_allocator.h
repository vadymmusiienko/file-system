#ifndef __SLAB_ALLOCATOR_H__
#define __SLAB_ALLOCATOR_H__

#include "sys/mem/mem.h"

#define PAGE_SIZE 4096

void *free_ptr;

// for allocators... in mm you will extend this naive implementation!
typedef struct slab_allocator {
    int objsize;
} slab_allocator_t;

void slab_init();

void slab_allocator_init(slab_allocator_t *allocator, int size);

void *slab_obj_alloc(slab_allocator_t allocator);

void slab_obj_free(slab_allocator_t allocator, void *obj);

void *page_alloc_n(int npages);

void page_free_n(void *ptr, int npages);

#endif // __SLAB_ALLOCATOR_H__
