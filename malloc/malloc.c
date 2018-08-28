#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>

#include "malloc.h"
#include "arena.h"

#define PAGESIZE					(0x1000)
#define ALIGN_DOWN(base, size)		((base) & -((__typeof__ (base)) (size)))
#define ALIGN_UP(base, size)		ALIGN_DOWN ((base) + (size) - 1, (size))

#define HEAP_MIN_SIZE				(32 * 1024)
#define HEAP_MAX_SIZE				(1024 * 1024)
#define DEFAULT_MMAP_THRESHOLD		(128 * 1024)

#define PREV_INUSE_BIT				(0x1)
#define PREV_INUSE(p)				((p)->size & PREV_INUSE_BIT)
#define INUSE_AT_OFFSET(p, s)		PREV_INUSE((mchunkptr)((void*)(p) + (s)))
#define SET_INUSE_AT_OFFSET(p, s)	((mchunkptr)((void*)(p) + (s)))->size |= PREV_INUSE_BIT
#define CLEAR_INUSE_AT_OFFSET(p, s)	((mchunkptr)((void*)(p) + (s)))->size &= ~(PREV_INUSE_BIT)
#define INUSE(p)					INUSE_AT_OFFSET((p), chunksize(p))
#define SET_INUSE(p)				SET_INUSE_AT_OFFSET((p), chunksize(p))
#define CLEAR_INUSE(p)				CLEAR_INUSE_AT_OFFSET((p), chunksize(p))

#define IS_MMAPPED_BIT				(0x2)
#define IS_MMAPPED(p)				((p)->size & IS_MMAPPED_BIT)

#define NON_MAIN_ARENA_BIT			(0x4)
#define NON_MAIN_ARENA(p)			((p)->size & NON_MAIN_ARENA_BIT)
#define SET_NON_MAIN_ARENA(p)		((p)->size |= NON_MAIN_ARENA_BIT)

#define SIZE_BITS					(PREV_INUSE_BIT | IS_MMAPPED_BIT | NON_MAIN_ARENA_BIT)
#define CHUNK_SIZE(p)				(CHUNK_SIZE_NOMASK(p) & ~(SIZE_BITS))
#define CHUNK_SIZE_NOMASK(p)		((p)->size)
#define PREV_SIZE(p)				((p)->prev_size)
#define SET_HEAD_SIZE(p, s)			((p)->size = (((p)->size & SIZE_BITS) | (s)))
#define SET_HEAD(p, s)				((p)->size = (s))
#define SET_PREV_SIZE(p, sz) 		((p)->prev_size = (sz))
#define SET_FOOT(p, s)				(CHUNK_AT_OFFSET((p),(s))->prev_size = (s))

#define NEXT_CHUNK(p)				((mchunkptr)((void*)(p) + CHUNK_SIZE(p)))
#define PREV_CHUNK(p)				((mchunkptr)((void*)(p) - PREV_SIZE(p)))
#define CHUNK_AT_OFFSET(p, s)		((mchunkptr)((void*)(p) + (s)))

#define CHUNK2MEM(p)				((void*)(p) + SIZE_SZ*2)
#define MEM2CHUNK(mem)				((mchunkptr)((void*)(mem) - SIZE_SZ*2))

#define BIN_AT(m, i)				((mbinptr)((void*)&((m)->bins[((i)-1) * 2]) - offsetof(struct malloc_chunk, fd)))
#define UNSORTED_CHUNKS(M)			(BIN_AT(M, 1))
#define INITIAL_TOP(M)				(UNSORTED_CHUNKS(M))

#define IN_SMALLBIN_RANGE(sz)		((sz) < MIN_LARGE_SIZE)

#define FIRST(b)					((b)->fd)
#define LAST(b)						((b)->bk)

#define LINK(p, fwd, bck) \
	do { \
		(p)->fd = (fwd); \
		(p)->bk = (bck); \
		(bck)->fd = (fwd)->bk = (p); \
	} while(0);

#define NONCONTIGUOUS_BIT			(2U)
#define CONTIGUOUS(M)				(!((M)->flags & NONCONTIGUOUS_BIT))
#define SET_NONCONTIGUOUS(M)		((M)->flags |= NONCONTIGUOUS_BIT)
#define SET_CONTIGUOUS(M)			((M)->flags &= ~NONCONTIGUOUS_BIT)

#define HEAP_FOR_PTR(ptr)			((hinfo) ((uint64_t) (ptr) & ~(HEAP_MAX_SIZE - 1)))
#define ARENA_FOR_CHUNK(ptr)		(NON_MAIN_ARENA(ptr) ? HEAP_FOR_PTR(ptr)->ar_ptr : &main_arena)

struct malloc_state main_arena;
static struct malloc_par mp = {
	.mmap_threshold = DEFAULT_MMAP_THRESHOLD
};

static void *_int_malloc(mstate av, size_t bytes);
static void _int_free(mstate av, mchunkptr p);
static void *_int_realloc(mstate av, mchunkptr oldp, size_t oldsize, size_t nb);
static void malloc_init_state(mstate av);
static void *sysmalloc(mstate av, size_t nb);
static void _alloc_split(mstate av, mchunkptr p, size_t nb);
static void *_alloc_top(mstate av, size_t nb);
static void link_bins(mstate av, mchunkptr p);
static void unlink_freelist(mchunkptr p);
static void munmap_chunk(mchunkptr p);

void *malloc(size_t bytes) {
	mchunkptr victim;
	mstate ar_ptr;
	void *mem;

	ar_ptr = arena_get(bytes);
	mem = _int_malloc(ar_ptr, bytes);
	arena_release(ar_ptr);

	if(!mem)
		return NULL;

	victim = MEM2CHUNK(mem);
	assert(IS_MMAPPED(victim) || ar_ptr == ARENA_FOR_CHUNK(victim));

	return mem;
}

void free(void *mem) {
	mchunkptr p;
	mstate ar_ptr;

	if(!mem)
		return;

	p = MEM2CHUNK(mem);

	if(IS_MMAPPED(p)){
		munmap_chunk(p);
		return;
	}

	ar_ptr = ARENA_FOR_CHUNK(p);
	arena_aquire(ar_ptr);
	_int_free(ar_ptr, p);
	arena_release(ar_ptr);
}

void *realloc(void *oldmem, size_t bytes){
	mstate ar_ptr;
	mchunkptr oldp;
	size_t oldsize, nb;
	void *newmem;

	if(!oldmem)
		return malloc(bytes);

	oldp = MEM2CHUNK(oldmem);
	oldsize = CHUNK_SIZE(oldp);

	nb = REQUEST2SIZE(bytes);

	if(IS_MMAPPED(oldp)){
		if(oldsize - SIZE_SZ >= nb)
			return oldmem;

		newmem = malloc(bytes);
		if(!newmem)
			return NULL;

		memcpy(newmem, oldmem, oldsize - SIZE_SZ);
		munmap_chunk(oldp);

		return newmem;
	}

	ar_ptr = arena_get(bytes);
	newmem = _int_realloc(ar_ptr, oldp, oldsize, nb);
	arena_release(ar_ptr);

#ifdef DEBUG
	printf("realloc : %p\n", newmem);
#endif
	return newmem;
}

void *calloc(size_t n, size_t elem_size){
	size_t bytes;
	mchunkptr victim;
	mstate ar_ptr;
	void *mem;

	bytes = n * elem_size;
	ar_ptr = arena_get(bytes);
	mem = _int_malloc(ar_ptr, bytes);
	arena_release(ar_ptr);

	if(!mem)
		return NULL;

	victim = MEM2CHUNK(mem);
	assert(IS_MMAPPED(victim) || ar_ptr == ARENA_FOR_CHUNK(victim));

#ifdef DEBUG
	printf("calloc : %p\n", mem);
#endif
	memset(mem, 0, bytes);
	return mem;
}

static void *_int_malloc(mstate av, size_t bytes){
	mchunkptr victim;
	void *mem;

	size_t size, nb;
	unsigned idx;
	mbinptr bin;

	nb = REQUEST2SIZE(bytes);

	if(av && !av->top)
		malloc_init_state(av);

	if(IN_SMALLBIN_RANGE(nb)){
		idx = SMALLBIN_INDEX(nb);
		bin = BIN_AT(av, idx);

		if((victim = LAST(bin)) && victim != bin){
			unlink_freelist(victim);

			SET_INUSE_AT_OFFSET(victim, nb);
			goto alloc_complete;
		}
		idx++;
	}
	else
		idx = LARGEBIN_INDEX(nb);

	while((victim = UNSORTED_CHUNKS(av)->bk) != UNSORTED_CHUNKS(av)){
		unlink_freelist(victim);

		size = CHUNK_SIZE(victim);
		if(size == nb){
			SET_INUSE_AT_OFFSET(victim, size);
			goto alloc_complete;
		}

		link_bins(av, victim);
	}

	if(!IN_SMALLBIN_RANGE(nb)){
		bin = BIN_AT(av, idx);

		if((victim = FIRST(bin)) == bin || CHUNK_SIZE(victim) < nb)
			goto next_bin;

		do {
			victim = victim->bk_nextsize;
		} while((size = CHUNK_SIZE(victim)) < nb);

		if(victim != LAST(bin) && CHUNK_SIZE_NOMASK(victim) == CHUNK_SIZE_NOMASK(victim->fd))
			victim = victim->fd;

		unlink_freelist(victim);
		_alloc_split(av, victim, nb);

		goto alloc_complete;
	}

next_bin:
	for(; idx<NBINS; idx++){
		bin = BIN_AT(av, idx);

		if((victim = FIRST(bin)) == bin)
			continue;

		if(idx >= NSMALLBINS){
			if(CHUNK_SIZE(victim) < nb)
				continue;

			do {
				victim = victim->bk_nextsize;
			} while((size = CHUNK_SIZE(victim)) < nb);

			if(victim != LAST(bin) && CHUNK_SIZE_NOMASK(victim) == CHUNK_SIZE_NOMASK(victim->fd))
				victim = victim->fd;
		}

		unlink_freelist(victim);
		_alloc_split(av, victim, nb);

		goto alloc_complete;
	}

	if(!(victim = _alloc_top(av, nb)))
		victim = sysmalloc(av, nb);

alloc_complete:
	if(av != &main_arena)
		SET_NON_MAIN_ARENA(victim);

#ifdef DEBUG
	printf("_int_malloc(0x%0x) : %p\n", nb, victim);
#endif

	mem = CHUNK2MEM(victim);
	return mem;
}

static void _int_free(mstate av, mchunkptr p){
	mchunkptr nextchunk;
	size_t size, prevsize, nextsize;

	size = CHUNK_SIZE(p);
	nextchunk = CHUNK_AT_OFFSET(p, size);
	nextsize = CHUNK_SIZE(nextchunk);

#ifdef DEBUG
	printf("_int_free(0x%0x) : %p\n", size, p);
#endif

	if(!PREV_INUSE(nextchunk)){
		abort();
	}

	if(!PREV_INUSE(p)){
		prevsize = PREV_SIZE(p);
		size += prevsize;
		p = CHUNK_AT_OFFSET(p, -prevsize);
		unlink_freelist(p);
	}

	if(nextchunk == av->top){
		size += nextsize;
		SET_HEAD(p, size | PREV_INUSE_BIT);
		av->top = p;
	}
	else{
		mchunkptr fwd, bck;

		if(INUSE_AT_OFFSET(nextchunk, nextsize))
			CLEAR_INUSE_AT_OFFSET(nextchunk, 0);
		else{
			unlink_freelist(nextchunk);
			size += nextsize;
		}

		SET_HEAD(p, size | PREV_INUSE_BIT);
		SET_FOOT(p, size);

		if(!IN_SMALLBIN_RANGE(size))
			p->fd_nextsize = p->bk_nextsize = NULL;

		bck = UNSORTED_CHUNKS(av);
		fwd = bck->fd;
#ifdef DEBUG
		printf("[%s]link : %p\n", __func__, p);
#endif
		LINK(p, fwd, bck);
	}
}

static void *_int_realloc(mstate av, mchunkptr oldp, size_t oldsize, size_t nb){
	mchunkptr newp, nextchunk;
	size_t nextsize;

	assert(oldp && CHUNK_SIZE(oldp) >= MINSIZE && oldsize < av->system_mem);

	nextchunk = CHUNK_AT_OFFSET(oldp, oldsize);
	nextsize = CHUNK_SIZE(nextchunk);
	newp = NULL;

	if(oldsize >= nb)
		newp = oldp;
	else if(nextchunk == av->top){
		if(oldsize + nextsize < nb + MINSIZE)
			goto newalloc;
		if(!_alloc_top(av, nb - oldsize))
			goto newalloc;

		newp = oldp;
		SET_HEAD_SIZE(newp, nb);
	}
	else if(!INUSE_AT_OFFSET(nextchunk, nextsize)){
		size_t newsize;

		if((newsize = oldsize + nextsize) < nb)
			goto newalloc;

		newp = oldp;
		unlink_freelist(nextchunk);
		SET_HEAD_SIZE(newp, newsize);
	}

	if(newp){
		_alloc_split(av, newp, nb);
		return CHUNK2MEM(newp);
	}

newalloc:
	do{
		void *newmem;

		newmem = _int_malloc(av, nb - MALLOC_ALIGN_MASK);
		if(!newmem)
			return NULL;

		assert(oldsize < nb);
		memcpy(newmem, CHUNK2MEM(oldp), oldsize - SIZE_SZ);
		_int_free(av, oldp);

		return newmem;
	} while(0);
}

static void malloc_init_state(mstate av){
	int i;
	mbinptr bin;

	for(i=1; i<NBINS; i++){
		bin = BIN_AT(av, i);
		bin->fd = bin->bk = bin;
	}

	SET_NONCONTIGUOUS(av);
	av->top = INITIAL_TOP(av);
}

static void *sysmalloc(mstate av, size_t nb){
	mchunkptr old_top;
	size_t old_size;
	void *old_end;

	size_t size;
	void *brk;
	mchunkptr p;

	if(!av || nb > mp.mmap_threshold){
		void *mm;
		int64_t correction;
		size_t front_misalign;

		size = ALIGN_UP(nb + MINSIZE, PAGESIZE);
		if(size < nb)
			goto try_brk;

		mm = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if(mm == MAP_FAILED)
			goto try_brk;

		front_misalign = (size_t)CHUNK2MEM(mm) & MALLOC_ALIGN_MASK;
		correction = front_misalign ? MALLOC_ALIGNMENT - front_misalign : 0;

		p = (mchunkptr)(mm + correction);
		SET_PREV_SIZE(p, correction);
		SET_HEAD(p, (size - correction) | IS_MMAPPED_BIT);

		//check_chunk(av, p);
		return p;
	}

try_brk:
	if(!av)
		return NULL;

	old_top = av->top;
	old_size = CHUNK_SIZE(old_top);
	old_end = (void*)(CHUNK_AT_OFFSET(old_top, old_size));
	brk = (void*) -1;

	assert(old_size < nb + MINSIZE);

	if(av == &main_arena){
		size = nb + MINSIZE;

		if(CONTIGUOUS(av))
			size -= old_size;
		size = ALIGN_UP(size, PAGESIZE);

		if(size > 0)
			brk = sbrk(size);
		if(brk == (void*)-1)
			return NULL;

		av->system_mem += size;

		if(brk == old_end)
			SET_HEAD(old_top, (size + old_size) | PREV_INUSE_BIT);
		else{
			int64_t correction;
			void *snd_brk, *aligned_brk;
			size_t front_misalign, end_misalign;

			snd_brk = (void*)-1;
			aligned_brk = brk;

			if(CONTIGUOUS(av)){
				if(old_size)
					av->system_mem += brk - old_end;

				front_misalign = (size_t)CHUNK2MEM(brk) & MALLOC_ALIGN_MASK;
				if(front_misalign){
					correction = MALLOC_ALIGNMENT - front_misalign;
					aligned_brk += correction;
				}
				else
					correction = 0;

				correction += old_size;
				end_misalign = (size_t)(brk + size + correction);
				correction += (ALIGN_UP (end_misalign, PAGESIZE)) - end_misalign;

				snd_brk = sbrk(correction);
			}
			else{
				front_misalign = (size_t)CHUNK2MEM(brk) & MALLOC_ALIGN_MASK;
				if(front_misalign)
					aligned_brk += MALLOC_ALIGNMENT - front_misalign;
			}

			if(snd_brk == (void*)-1){
				correction = 0;
				snd_brk = sbrk(0);
			}

			av->top = (mchunkptr)aligned_brk;
			SET_HEAD(av->top, (snd_brk - aligned_brk + correction) | PREV_INUSE_BIT);
			av->system_mem += correction;
		}
	}
	else{
		// TODO
	}

	p = _alloc_top(av, nb);

	//check_chunk(av, p);
	return p;
}

static void _alloc_split(mstate av, mchunkptr p, size_t nb){
	size_t size, remainder_size;

	size = CHUNK_SIZE(p);
	assert(size >= nb);

	remainder_size = size - nb;
	if(remainder_size < MINSIZE)
		SET_INUSE_AT_OFFSET(p, size);
	else{
		mchunkptr remainder, fwd, bck;
		int inuse;

		inuse = INUSE_AT_OFFSET(p, size);

		remainder = CHUNK_AT_OFFSET(p, nb);
		SET_HEAD(p, nb | PREV_INUSE_BIT);
		SET_HEAD(remainder, remainder_size | PREV_INUSE_BIT);
		SET_FOOT(remainder, remainder_size);

		if(inuse)
			_int_free(av, remainder);
		else{
			if(!IN_SMALLBIN_RANGE(remainder_size))
				remainder->fd_nextsize = remainder->bk_nextsize = NULL;

			bck = UNSORTED_CHUNKS(av);
			fwd = bck->fd;
#ifdef DEBUG
			printf("[%s]link : %p\n", __func__, p);
#endif
			LINK(remainder, fwd, bck);
		}
	}
}

static void *_alloc_top(mstate av, size_t nb){
	mchunkptr victim;
	size_t size;
	size_t remainder_size;
	mchunkptr remainder;

	victim = av->top;
	size = CHUNK_SIZE(victim);

	if(size < nb + MINSIZE)
		return NULL;

	remainder_size = size - nb;
	remainder = CHUNK_AT_OFFSET(victim, nb);

	av->top = remainder;
	SET_HEAD(victim, nb | PREV_INUSE_BIT | (av != &main_arena ? NON_MAIN_ARENA_BIT : 0));
	SET_HEAD(remainder, remainder_size | PREV_INUSE_BIT);

	return victim;
}

static void link_bins(mstate av, mchunkptr p){
	mchunkptr fwd, bck;
	size_t size;
	unsigned idx;

	size = CHUNK_SIZE(p);
	if(IN_SMALLBIN_RANGE(size)){
		idx = SMALLBIN_INDEX(size);
		bck = BIN_AT(av, idx);
		fwd = bck->fd;
	}
	else{
		idx = LARGEBIN_INDEX(size);
		bck = BIN_AT(av, idx);
		fwd = bck->fd;

		if(fwd == bck){
			p->fd_nextsize = p->bk_nextsize = p;
			goto link;
		}

		if(size < CHUNK_SIZE(bck->bk)){
			fwd = bck;
			bck = fwd->bk;

			p->fd_nextsize = fwd->fd;
			p->bk_nextsize = fwd->fd->bk_nextsize;
			p->fd_nextsize->bk_nextsize = p->bk_nextsize->fd_nextsize = p;

			goto link;
		}

		while(size < CHUNK_SIZE(fwd))
			fwd = fwd->fd_nextsize;

		if(size == CHUNK_SIZE(fwd)){
			fwd = fwd->fd;
			p->fd_nextsize = p->bk_nextsize = NULL;
		}
		else{
			p->fd_nextsize = fwd;
			p->bk_nextsize = fwd->bk_nextsize;
			p->fd_nextsize->bk_nextsize = p->bk_nextsize->fd_nextsize = p;
		}
		bck = fwd->bk;
	}

link:
#ifdef DEBUG
	printf("[%s]link : %p\n", __func__, p);
#endif
	LINK(p, fwd, bck);
}

static void unlink_freelist(mchunkptr p){
	mchunkptr fwd, bck;

	assert(CHUNK_SIZE(p) == PREV_SIZE(NEXT_CHUNK(p)));

	fwd = p->fd;
	bck = p->bk;

#ifdef DEBUG
	printf("[%s]unlink : %p\n", __func__, p);
#endif
	assert(fwd->bk == p && bck->fd == p);

	bck->fd = fwd;
	fwd->bk = bck;

	if(!IN_SMALLBIN_RANGE(CHUNK_SIZE_NOMASK(p)) && p->fd_nextsize){
		mchunkptr fwd_ns, bck_ns;

		fwd_ns = p->fd_nextsize;
		bck_ns = p->bk_nextsize;

		assert(fwd_ns->bk_nextsize == p && bck_ns->fd_nextsize == p);

		if(fwd->fd_nextsize){
			fwd_ns->bk_nextsize = bck_ns;
			bck_ns->fd_nextsize = fwd_ns;
		}
		else if (p->fd_nextsize == p)
			fwd->fd_nextsize = fwd->bk_nextsize = fwd;
		else{
			fwd->fd_nextsize = fwd_ns;
			fwd->bk_nextsize = bck_ns;
			fwd_ns->bk_nextsize = bck_ns->fd_nextsize = fwd;
		}
	}
}

static void munmap_chunk(mchunkptr p){
	void *block;
	size_t total_size;

	block = (void*)p - PREV_SIZE(p);
	total_size = CHUNK_SIZE(p) + PREV_SIZE(p);

	munmap(block, total_size);
}
