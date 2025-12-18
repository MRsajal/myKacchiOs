#ifndef MEM_H
#define MEM_H

#include "types.h"

void mem_init(void);

void *mem_alloc(size_t size);
void mem_free(void *ptr);

#endif