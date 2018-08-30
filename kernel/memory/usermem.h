#ifndef _USERMEM_H
#define _USERMEM_H

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define copy_to_user(dst, src, size)   (((dst) < (1UL<<39)-1) ? (uint64_t)memcpy((void*)dst, src, size) : -1)
#define copy_from_user(dst, src, size) (((src) < (1UL<<39)-1) ? memcpy(dst, (void*)src, size) : (void*)-1)

uint64_t brk;

int prepare_user(void);
uint64_t mmap_in_user(uint64_t vaddr, uint64_t paddr, size_t length, int prot);
uint64_t mmap_user(uint64_t vaddr, size_t length, int prot);
uint64_t mprotect_user(uint64_t vaddr, size_t length, int prot);
uint64_t munmap_user(uint64_t vaddr, size_t length);

#endif
