#ifndef _MEM_MANAGE_H
#define _MEM_MANAGE_H

#include <stdint.h>

void init_gmem_manage(size_t mem_size);
uint64_t palloc(uint64_t addr, size_t size);
int pfree(uint64_t addr);
uint64_t get_gmem_info(int menu);

#endif
