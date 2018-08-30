#ifndef _TRANSLATION_H
#define _TRANSLATION_H

#include <stdint.h>
#include "vm/vm.h"

//uint64_t translate(int vcpufd, uint64_t addr, int writeable, int user);
uint64_t translate(struct vm *vm, uint64_t pml4_addr, uint64_t addr, int write, int user);

#endif
