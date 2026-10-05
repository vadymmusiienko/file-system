#include "include/slab_allocator.h"

void slab_init() {
    free_ptr = dram;
}

void slab_allocator_init(slab_allocator_t *allocator, int size) {
    allocator->objsize = size;
}

void *slab_obj_alloc(slab_allocator_t allocator) {
    void *ret = free_ptr;
    free_ptr += allocator.objsize;
    return ret;
}

void slab_obj_free(slab_allocator_t allocator, void *obj) {
    // do nothing (for now)!
}

void *page_alloc_n(int npages) {
    void *ret = free_ptr;
    free_ptr += npages * PAGE_SIZE;
    return ret;
}

void page_free_n(void *ptr, int npages) {
    // do nothing (for now)!
}
