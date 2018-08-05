#ifndef _VM_H
#define _VM_H

#include <stddef.h>

#define check_addr(vm, addr)		(addr < vm->mem_size)
#define assert_addr(vm, addr)		assert(addr < vm->mem_size)
#define guest2phys(vm, addr)		(vm->mem + addr)

struct vcpu {
	int fd;
	struct kvm_run *run;
};

struct vm {
	int fd;
	int vmfd;

	size_t mem_size;
	void *mem;

	unsigned ncpu;
	struct vcpu vcpu[];
};

#endif
