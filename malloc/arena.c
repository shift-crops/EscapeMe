#include <stdlib.h>
#include "arena.h"

extern struct malloc_state main_arena;

#define mutex_lock(mutex)	do { if(__sync_bool_compare_and_swap(mutex, 0, 1)) break; } while (1);
#define mutex_unlock(mutex)	(*mutex = 0)

mstate arena_get(size_t bytes){
	mstate av = &main_arena;

	arena_aquire(av);
	return av;
}

void arena_aquire(mstate av){
	if(av)
		mutex_lock(&av->mutex);
}

void arena_release(mstate av){
	if(av)
		mutex_unlock(&av->mutex);
}
