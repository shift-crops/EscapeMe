#ifndef _ARENA_H
#define _ARENA_H

#include <stdlib.h>
#include "malloc.h"

struct heap_info {
	mstate ar_ptr;
	size_t size;
};
typedef struct heap_info* hinfo;

mstate arena_get(size_t bytes);
void arena_aquire(mstate ar);
void arena_release(mstate ar);

#endif
