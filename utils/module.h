#ifndef _MODULE_H
#define _MODULE_H

int init_modules(unsigned nmod, char *list[]);
void fini_modules(void);
int load_module(struct vm *vm, int id, uint64_t addr, off_t offset, size_t size);
int load_kernel(struct vm *vm);

#endif
