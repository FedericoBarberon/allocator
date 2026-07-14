#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include<stddef.h>

void *memalloc(size_t size);

void *memrealloc(void *ptr, size_t new_size);

void memfree(void *ptr);

#endif
