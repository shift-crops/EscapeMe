#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include "bits.h"
#include "vm/vm.h"
#include "vm/kvm_handler.h"
#include "utils/palloc.h"
#include "utils/translate.h"
#include "utils/debug.h"

static int init_vcpu(int fd, struct vm *vm);
static int init_memory(struct vm *vm);
static int set_long_mode(struct vm *vm, int vcpufd);

struct vm *init_vm(unsigned ncpu, size_t mem_size){
	struct vm *vm = NULL;
	int fd, vmfd;

	if(ncpu < 1 || ncpu > 4)
		return NULL;

	if((fd = open("/dev/kvm", O_RDONLY)) < 0){
		perror("open /dev/kvm");
		goto end;
	}

	if((vmfd = ioctl(fd, KVM_CREATE_VM, 0)) < 0){
		perror("ioctl KVM_CREATE_VM");
		goto end;
	}

	if(!(vm = (struct vm*)calloc(sizeof(struct vm)+sizeof(struct vcpu)*ncpu, 1))){
		perror("malloc (struct vm)");
		goto end;
	}

	vm->vmfd 		= vmfd;
	vm->ncpu 		= ncpu;
	vm->mem_size 	= mem_size;
	init_gmem_manage(mem_size);

	if(init_vcpu(fd, vm) < ncpu || init_memory(vm) < 0){
		free(vm);
		vm = NULL;
	}

end:
	close(fd);
	return vm;
}

static int init_vcpu(int fd, struct vm *vm){
	size_t mmap_size;
	int i;

	if((mmap_size = ioctl(fd, KVM_GET_VCPU_MMAP_SIZE, NULL)) <= 0){
		perror("ioctl KVM_GET_VCPU_MMAP_SIZE");
		return -1;
	}

	for(i = 0; i < vm->ncpu; i++){
		struct vcpu *vcpu = &vm->vcpu[i];
		int vcpufd;
		struct kvm_run *run;

		if((vcpufd = ioctl(vm->vmfd, KVM_CREATE_VCPU, i)) < 0){
			perror("ioctl KVM_CREATE_VCPU");
			break;
		}

		run = (struct kvm_run *)mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, vcpufd, 0);
		if(run == MAP_FAILED){
			perror("mmap (struct kvm_run)");
			break;
		}

		vcpu->fd  = vcpufd;
		vcpu->run = run;
	}

	return i;
}

static int init_memory(struct vm *vm){
	size_t mem_size = vm->mem_size;
	void *mem;

	mem = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
	if (mem == MAP_FAILED) {
		perror("mmap mem");
		return -1;
	}

	vm->mem = mem;
	madvise(mem, mem_size, MADV_MERGEABLE);

	struct kvm_userspace_memory_region region = {
		.slot = 0,
		.flags = 0,
		.guest_phys_addr = 0,
		.memory_size = mem_size,
		.userspace_addr = (uint64_t)mem
	};

	if (ioctl(vm->vmfd, KVM_SET_USER_MEMORY_REGION, &region) < 0){
		perror("ioctl KVM_SET_USER_MEMORY_REGION");
		goto error;
	}

	return 0;

error:
	munmap(mem, mem_size);
	return -1;
}

int run_vm(struct vm *vm, unsigned vcpuid, uint64_t entry){
	if(vcpuid < 0 || vcpuid >= vm->ncpu)
		return -1;

	struct vcpu *vcpu = &vm->vcpu[vcpuid];
	int vcpufd = vcpu->fd;
	struct kvm_run *run = vcpu->run;

	struct kvm_guest_debug debug = {
		.control	= KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP,
	};

	if (ioctl(vcpufd, KVM_SET_GUEST_DEBUG, &debug) < 0){
		perror("KVM_SET_GUEST_DEBUG");
		return -1;
	}

	if(set_long_mode(vm, vcpufd) < 0)
		return -1;

	struct kvm_regs regs = {
		.rip = entry,
		.rflags = 0x02,
	};

	if(ioctl(vcpufd, KVM_SET_REGS, &regs) < 0){
		perror("ioctl KVM_SET_REGS");
		return -1;
	}

	for(;;){
		struct kvm_regs regs;
		struct kvm_sregs sregs;
		uint64_t gaddr;

		if(ioctl(vcpufd, KVM_RUN, 0) < 0) {
			perror("ioctl KVM_RUN");
			return -1;
		}

#ifdef DEBUG
		dump_regs(vcpufd);
#endif
		switch(run->exit_reason){
			case KVM_EXIT_HLT:
#ifdef DEBUG
				printf("HLT\n");
				getchar();
				break;
#else
				return 0;
#endif
			case KVM_EXIT_IO:
#ifdef DEBUG
				printf("IO\n");
#endif
				kvm_handle_io(vm, vcpu);
				break;
			case KVM_EXIT_DEBUG:
				if(ioctl(vcpufd, KVM_GET_REGS, &regs) < 0){
					perror("ioctl KVM_GET_REGS");
					return -1;
				}
				if(ioctl(vcpufd, KVM_GET_SREGS, &sregs) < 0){
					perror("ioctl KVM_GET_SREGS");
					return -1;
				}

				if((gaddr = translate(vm, sregs.cr3, regs.rip, 0, 0)) == -1)
					return 1;

				if(!memcmp(guest2phys(vm, gaddr), "\x0f\x01\xc1", 3) \
					|| !memcmp(guest2phys(vm, gaddr), "\x0f\x01\xd9", 3)){
					if(sregs.cs.dpl != 0)
						return 2;

#ifdef DEBUG
					printf("HYPERCALL\n");
#endif
					kvm_handle_hypercall(vm, vcpu);
					*(char*)guest2phys(vm, gaddr+2) = 0xd9;
				}
				else if(!memcmp(guest2phys(vm, gaddr), "\x0f\x05", 2)){
					if(sregs.cs.dpl != 3)
						return 2;

#ifdef DEBUG
					printf("SYSCALL\n");
#endif
					kvm_handle_syscall(vm, vcpu);
				}
				break;
			default:
				printf("exit_reason : %d\n", run->exit_reason);
#ifdef DEBUG
				getchar();
#endif
				return -1;
		}
	}

	return 0;
}

static int set_long_mode(struct vm *vm, int vcpufd){
	struct kvm_sregs sregs;
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11, 			// Code: execute, read, accessed
		.dpl = 0,
		.db = 0,
		.s = 1, 				// Code/data
		.l = 1,
		.g = 1, 				// 4KB granularity
	};

	if(ioctl(vcpufd, KVM_GET_SREGS, &sregs) < 0){
		perror("ioctl KVM_GET_SREGS");
		return -1;
	}

	sregs.cs = seg;
	seg.type = 3; 					// Data: read/write, accessed
	seg.selector = 2 << 3;
	sregs.ds = sregs.ss = seg;

	uint64_t pml4_addr 	= palloc(0, 0x1000);
	uint64_t pdpt_addr 	= palloc(0, 0x1000*(((vm->mem_size-1)>>39) + 1));
	uint64_t pd_addr 	= palloc(0, 0x1000*(((vm->mem_size-1)>>30) + 1));

	assert_addr(vm, pml4_addr);
	assert_addr(vm, pdpt_addr);
	assert_addr(vm, pd_addr);

	uint64_t *pml4	= guest2phys(vm, pml4_addr);
	uint64_t *pdpt	= guest2phys(vm, pdpt_addr);
	uint64_t *pd 	= guest2phys(vm, pd_addr);

	for(int i = 0; i < ((vm->mem_size-1)>>39 & 0x1ff) + 1; i++){
		for(int j = 0; j < ((vm->mem_size-1)>>30 & 0x1ff) + 1; j++){
			for(int k = 0; k < ((vm->mem_size-1)>>21 & 0x1ff) + 1; k++){
				pd[k+0x200*(j+0x200*i)] = PDE64_PRESENT | PDE64_RW | PDE64_PS | 0x200000*(k+0x200*(j+0x200*i));
			}
			pdpt[j+0x200*i] = PDE64_PRESENT | PDE64_RW | (pd_addr+0x1000*(j+0x200*i));
		}
		pml4[i] = PDE64_PRESENT | PDE64_RW | (pdpt_addr+0x1000*i);
	}

	sregs.cr3  = pml4_addr;
	sregs.cr4  = CR4_PAE;
	sregs.cr0  = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs.efer = EFER_LME | EFER_LMA;

	if(ioctl(vcpufd, KVM_SET_SREGS, &sregs) < 0){
		perror("ioctl KVM_SET_SREGS");
		return -1;
	}

	return 0;
}
