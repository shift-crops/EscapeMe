#ifndef _VM_H
#define _VM_H

#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#define check_addr(vm, addr)		(addr < vm->mem_size)
#define assert_addr(vm, addr)		assert(addr < vm->mem_size)
#define guest2phys(vm, addr)		(vm->mem + addr)

struct vcpu {
	int fd;
	struct kvm_run *run;
};

struct vm {
	int vmfd;

	size_t mem_size;
	void *mem;

	unsigned ncpu;
	struct vcpu vcpu[];
};

struct vm *init_vm(unsigned ncpu, size_t mem_size);
int load_image(struct vm *vm, int fd);
int run_vm(struct vm *vm, unsigned vcpuid, uint64_t entry);

#endif
