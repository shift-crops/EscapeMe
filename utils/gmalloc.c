#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define NBINS					128
#define NSMALLBINS				64

#define SMALLBIN_WIDTH			GMALLOC_ALIGNMENT

#define MIN_LARGE_SIZE			(NSMALLBINS * SMALLBIN_WIDTH)
#define IN_SMALLBIN_RANGE(sz)	((sz) < MIN_LARGE_SIZE)

#define SMALLBIN_INDEX(sz)		(((uint64_t)(sz)) >> 12)
#define LARGEBIN_INDEX(sz)	\
	(((((uint64_t) (sz)) >> 14) <= 48) ?  48 + (((uint64_t) (sz)) >> 14) :\
	 ((((uint64_t) (sz)) >> 17) <= 20) ?  91 + (((uint64_t) (sz)) >> 17) :\
	 ((((uint64_t) (sz)) >> 20) <= 10) ? 110 + (((uint64_t) (sz)) >> 20) :\
	 ((((uint64_t) (sz)) >> 23) <= 4) ? 119 + (((uint64_t) (sz)) >> 23) :\
	 ((((uint64_t) (sz)) >> 27) <= 2) ? 124 + (((uint64_t) (sz)) >> 27) : 126)

#define GMALLOC_ALIGNMENT		(1 << 12)
#define GMALLOC_ALIGN_MASK		(GMALLOC_ALIGNMENT - 1)
#define MINSIZE					GMALLOC_ALIGNMENT
#define REQUEST2SIZE(req) 	((req) < MINSIZE ? MINSIZE : ((req) + GMALLOC_ALIGN_MASK) & ~GMALLOC_ALIGN_MASK)

#define BIN_AT(av, idx) \
			((gmbinptr)((void*)&((av)->bins[(idx) * 2]) - offsetof(struct gmalloc_chunk, fd)))
#define FIRST(b)				((b)->fd)
#define LAST(b)					((b)->bk)

#define CHUNK_MEM(p)			((p)->addr)
#define CHUNK_SIZE(p)			((p)->size)
#define INUSE(p)				((p)->fd == NULL)


struct gmalloc_chunk {
	uint64_t addr;
	size_t size;
	struct gmalloc_chunk *fd, *bk;		// free list
	struct gmalloc_chunk *next, *prev;	// all chunks
};
typedef struct gmalloc_chunk *gmchunkptr, *gmbinptr;

struct gmalloc_state {
	unsigned initialized;

	gmchunkptr top;
	gmbinptr bins[NBINS*2];

	size_t inuse;
	size_t system_mem;
} arena;
typedef struct gmalloc_state *gmstate;

static gmchunkptr _int_gmalloc(gmstate av, size_t bytes);
static gmchunkptr _int_gmalloc_manual(gmstate av, uint64_t addr, size_t bytes);
static void _int_gfree(gmstate av, gmchunkptr p);

void init_gmem_manage(size_t mem_size){
	gmstate av = &arena;
	gmchunkptr top;

	top = (gmchunkptr)malloc(sizeof(struct gmalloc_chunk));
	top->addr = 0;
	top->size = mem_size;
	top->fd   = top->bk   = NULL;
	top->next = top->prev = NULL;

	av->top 		= top;
	av->inuse		= 0;
	av->system_mem 	= mem_size;

	for(int i = 0; i < NBINS; i++){
		gmbinptr bin = BIN_AT(av, i);
		bin->fd = bin->bk = bin;
	}

	av->initialized = 1;
}

uint64_t gmalloc(uint64_t addr, size_t bytes){
	gmstate av = &arena;
	gmchunkptr p;

	if(!av->initialized || bytes > av->system_mem - av->inuse)
		return -1;

	if((p = addr ? _int_gmalloc_manual(av, addr, bytes) : _int_gmalloc(av, bytes))){
		av->inuse += CHUNK_SIZE(p);
		return CHUNK_MEM(p);
	}

	return -1;
}

int gfree(uint64_t addr){
	gmstate av = &arena;
	gmchunkptr p;

	if(!av->initialized)
		return -1;

	for(p = av->top; p; p = p->prev)
		if(CHUNK_MEM(p) == addr){
			_int_gfree(av, p);
			av->inuse -= CHUNK_SIZE(p);
			return 0;
		}

	return -1;
}

uint64_t get_gmem_info(int menu){
	gmstate av = &arena;

	switch(menu){
		case 0:
			return av->initialized;
		case 1:
			return av->system_mem;
		case 2:
			return av->inuse;
	}
	return -1;
}

static void unlink_freelist(gmchunkptr p);
static void link_bins(gmstate av, gmchunkptr p);
static void _alloc_split(gmstate av, gmchunkptr p, size_t nb);
static gmchunkptr _alloc_top(gmstate av, size_t nb);

static gmchunkptr _int_gmalloc(gmstate av, size_t bytes){
	gmchunkptr victim;

	size_t nb;
	unsigned idx;
	gmbinptr bin;

	nb = REQUEST2SIZE(bytes);

	if(IN_SMALLBIN_RANGE(nb)){
		idx = SMALLBIN_INDEX(nb);
		bin = BIN_AT(av, idx);

		if((victim = LAST(bin)) && victim != bin){
			unlink_freelist(victim);
			goto alloc_complete;
		}

		idx++;
	}
	else {
		idx = LARGEBIN_INDEX(nb);
		bin = BIN_AT(av, idx);

		if((victim = FIRST(bin)) == bin || CHUNK_SIZE(victim) < nb)
			goto next_bin;

		for(victim = LAST(bin); CHUNK_SIZE(victim) < nb; victim = victim->bk);

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

			for(victim = LAST(bin); CHUNK_SIZE(victim) < nb; victim = victim->bk);
		}

		unlink_freelist(victim);
		_alloc_split(av, victim, nb);

		goto alloc_complete;
	}

	if(!(victim = _alloc_top(av, nb)))
		return NULL;

alloc_complete:
	return victim;
}

static gmchunkptr _int_gmalloc_manual(gmstate av, uint64_t addr, size_t bytes){
	gmchunkptr victim, tmp = NULL;
	size_t nb;

	if(addr & GMALLOC_ALIGN_MASK)
		return NULL;

	nb = REQUEST2SIZE(bytes);

	for(victim = av->top; victim && CHUNK_MEM(victim) > addr; victim = victim->prev);

	if(!victim)
		return NULL;

	if(victim == av->top){
		if(addr-CHUNK_MEM(victim) > 0){
			_alloc_split(av, victim, addr-CHUNK_MEM(victim));
			tmp = victim;
		}

		victim = _alloc_top(av, nb);
	}
	else {
		if(INUSE(victim) || CHUNK_MEM(victim)+CHUNK_SIZE(victim) < addr+nb)
			return NULL;

		if(addr-CHUNK_MEM(victim) > 0){
			_alloc_split(av, victim, addr-CHUNK_MEM(victim));
			tmp = victim;
			victim = victim->next;
		}

		_alloc_split(av, victim, nb);
	}

	if(tmp)
		_int_gfree(av, tmp);

	return victim;
}

static void _int_gfree(gmstate av, gmchunkptr p){
	gmchunkptr next, prev;

	next = p->next;
	prev = p->prev;

	if(!INUSE(prev)){
		unlink_freelist(prev);

		prev->next = next;
		next->prev = prev;
		CHUNK_SIZE(prev) += CHUNK_SIZE(p);

		free(p);
		p = prev;
		prev = p->prev;
	}

	if(next == av->top){
		gmchunkptr top = av->top;

		prev->next = top;
		top->prev  = prev;

		CHUNK_MEM(top)   = CHUNK_MEM(p);
		CHUNK_SIZE(top) += CHUNK_SIZE(p);

		free(p);
		return;
	}

	if (!INUSE(next)){
		unlink_freelist(next);

		prev->next = next;
		next->prev = prev;
		CHUNK_MEM(next)   = CHUNK_MEM(p);
		CHUNK_SIZE(next) += CHUNK_SIZE(p);

		free(p);
		p = next;
	}

	link_bins(av, p);
}

static void unlink_freelist(gmchunkptr p){
	gmchunkptr fwd, bck;

	fwd = p->fd;
	bck = p->bk;

	assert(fwd->bk == p && bck->fd == p);

	fwd->bk = bck;
	bck->fd = fwd;

	p->fd = p->bk = NULL;
}

static void link_bins(gmstate av, gmchunkptr p){
	gmchunkptr fwd, bck;
	size_t size;
	unsigned idx;

	assert(p->fd == NULL && p->bk == NULL);

	size = CHUNK_SIZE(p);
	if(IN_SMALLBIN_RANGE(size)){
		idx = SMALLBIN_INDEX(size);
		bck = BIN_AT(av, idx);
		fwd = bck->fd;
	}
	else {
		idx = LARGEBIN_INDEX(size);
		bck = BIN_AT(av, idx);
		fwd = bck->fd;

		if(fwd == bck)
			goto link;

		if(size < CHUNK_SIZE(bck->bk)){
			fwd = bck;
			bck = fwd->bk;
			goto link;
		}

		while(size < CHUNK_SIZE(fwd))
			fwd = fwd->fd;

		bck = fwd->bk;
	}

link:
	p->fd = fwd;
	p->bk = bck;
	bck->fd = fwd->bk = p;
}

static void _alloc_split(gmstate av, gmchunkptr p, size_t nb){
	size_t size, remainder_size;
	gmchunkptr remainder;

	size = CHUNK_SIZE(p);
	assert(nb && size >= nb);

	remainder_size = size - nb;
	if(!remainder_size)
		return;

	remainder = (gmchunkptr)malloc(sizeof(struct gmalloc_chunk));
	CHUNK_MEM(remainder)  = CHUNK_MEM(p) + nb;
	CHUNK_SIZE(remainder) = remainder_size;
	remainder->next = p->next;
	remainder->prev = p;

	p->size			= nb;
	p->next			= remainder;

	if(p == av->top){
		remainder->fd = remainder->bk = NULL;
		av->top = remainder;
	}
	else
		link_bins(av, remainder);
}

static gmchunkptr _alloc_top(gmstate av, size_t nb){
	gmchunkptr victim;

	victim = av->top;
	if(!victim || CHUNK_SIZE(victim) < nb)
		return NULL;

	_alloc_split(av, victim, nb);

	return victim;
}
