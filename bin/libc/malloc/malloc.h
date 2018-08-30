#ifndef __MALLOC_H
#define __MALLOC_H

#include <stddef.h>
#include <stdint.h>

#define SIZE_SZ			(sizeof(size_t))

#define NBINS			128
#define NFASTBINS		(FASTBIN_INDEX(REQUEST2SIZE(MAX_FAST_SIZE)) + 1)
#define NSMALLBINS		64

#define MAX_FAST_SIZE		(SIZE_SZ * 80 / 4)
#define SMALLBIN_WIDTH		MALLOC_ALIGNMENT
#define SMALLBIN_CORRECTION	(MALLOC_ALIGNMENT > 2 * SIZE_SZ)
#define MIN_LARGE_SIZE		((NSMALLBINS - SMALLBIN_CORRECTION) * SMALLBIN_WIDTH)

#define FASTBIN_INDEX(sz)	((((unsigned int)(sz)) >> (SIZE_SZ == 8 ? 4 : 3)) - 2)
#define SMALLBIN_INDEX(sz)	((SMALLBIN_WIDTH == 16 ? (((unsigned)(sz)) >> 4) : (((unsigned) (sz)) >> 3)) + SMALLBIN_CORRECTION)
#define LARGEBIN_INDEX(sz)	\
	(((((uint64_t) (sz)) >> 6) <= 48) ?  48 + (((uint64_t) (sz)) >> 6) :\
	 ((((uint64_t) (sz)) >> 9) <= 20) ?  91 + (((uint64_t) (sz)) >> 9) :\
	 ((((uint64_t) (sz)) >> 12) <= 10) ? 110 + (((uint64_t) (sz)) >> 12) :\
	 ((((uint64_t) (sz)) >> 15) <= 4) ? 119 + (((uint64_t) (sz)) >> 15) :\
	 ((((uint64_t) (sz)) >> 18) <= 2) ? 124 + (((uint64_t) (sz)) >> 18) : 126)

#define MALLOC_ALIGNMENT	(SIZE_SZ*2 < __alignof__(long double) ? __alignof__(long double) : SIZE_SZ*2)
#define MALLOC_ALIGN_MASK	(MALLOC_ALIGNMENT - 1)
#define MIN_CHUNK_SIZE		(offsetof(struct malloc_chunk, fd_nextsize))
#define MINSIZE  		((uint64_t)((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))
#define REQUEST2SIZE(req) \
	(((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE) ? MINSIZE : ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

struct malloc_chunk {
	size_t prev_size;
	size_t size;

	struct malloc_chunk *fd, *bk;
	struct malloc_chunk *fd_nextsize, *bk_nextsize;
};
typedef struct malloc_chunk *mchunkptr, *mfastbinptr, *mbinptr;

struct malloc_state {
	char mutex;
	int flags;

	mfastbinptr fastbinsY[NFASTBINS];

	mchunkptr top;
	mchunkptr last_reminder;
	mbinptr bins[NBINS*2];

	struct malloc_state *next;
	size_t system_mem;
};
typedef struct malloc_state *mstate;

struct malloc_par {
	size_t mmap_threshold;
};

#endif
