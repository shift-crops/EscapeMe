#ifndef _PAGINT_H
#define _PAGINT_H

#include <stdint.h>
#include "vm/vm.h"

uint64_t paging(struct vm *vm, uint64_t pml4_addr, uint64_t addr);

#endif
