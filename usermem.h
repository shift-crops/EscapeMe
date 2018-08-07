#ifndef _USERMEM_H
#define _USERMEM_H

#include <stdint.h>
#include <sys/types.h>

int prepare_user(void);
uint64_t mmap_user(uint64_t addr, size_t length);
uint64_t munmap_user(uint64_t addr, size_t length);

#endif
