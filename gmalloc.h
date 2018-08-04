#ifndef _MEM_MANAGE_H
#define _MEM_MANAGE_H

void init_gmem_manage(size_t mem_size);
unsigned long gmalloc(unsigned long addr, size_t size);
int gfree(unsigned long addr);
unsigned long get_gmem_remain(void);

#endif
