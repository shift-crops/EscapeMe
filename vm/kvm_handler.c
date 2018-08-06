#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include "kvm_handler.h"
#include "vm.h"
#include "utils/gmalloc.h"
#include "utils/translate.h"

int kvm_handle_io(struct vm *vm, struct vcpu *vcpu){
	/*
	int vcpufd = vcpu->fd;
	struct kvm_run *run = vcpu->run;
	 */
	return 0;
}

int kvm_handle_hypercall(struct vm *vm, struct vcpu *vcpu){
	struct kvm_regs regs;
	struct kvm_sregs sregs;
	int vcpufd = vcpu->fd;
	uint64_t gaddr;
	unsigned long ret = -1;

	if(ioctl(vcpufd, KVM_GET_REGS, &regs) < 0){
		perror("ioctl KVM_GET_REGS");
		return -1;
	}
	if(ioctl(vcpufd, KVM_GET_SREGS, &sregs) < 0){
		perror("ioctl KVM_GET_SREGS");
		return -1;
	}

	unsigned nr = regs.rax;
	unsigned long arg[] = {regs.rbx, regs.rcx, regs.rdx, regs.rsi};

	//printf("nr : %d\n", nr);
	switch(nr){
		case 0x10:		// read(0, buf, size)
			if((gaddr = translate(vcpufd, arg[0])))
				ret = read(STDIN_FILENO, guest2phys(vm, gaddr), arg[1]);
			break;
		case 0x11:		// write(1, buf, size)
			if((gaddr = translate(vcpufd, arg[0])))
				ret = write(STDOUT_FILENO, guest2phys(vm, gaddr), arg[1]);
			break;
		case 0x20:
			ret = get_gmem_info(arg[0]);
			break;
		case 0x21:		// gmalloc(addr, size)
			if((ret = gmalloc(arg[0], arg[1])) != -1)
				assert_addr(vm, ret);
			break;
		case 0x22:		// gfree(addr);
			if(check_addr(vm, arg[0]))
				ret = gfree(arg[0]);
			break;
	}

	regs.rax  = ret;
	regs.rip += 3;

	if(ioctl(vcpufd, KVM_SET_REGS, &regs) < 0){
		perror("ioctl KVM_SET_REGS");
		return -1;
	}

	return 0;
}

#define MSR_STAR	0xc0000081;
#define MSR_LSTAR	0xc0000082;
#define MSR_SFMASK	0xc0000084;

/*
#define base(s)		((((s)>>16)&0xffffff) | ((((s)>>56)&0xff)<<24))
#define limit(s)	(((s)&0xffff) | ((((s)>>48)&0xf)<<16))
#define type(s)		(((s)>>40)&0xf)
#define s(s)		(((s)>>44)&1)
#define dpl(s)		(((s)>>45)&3)
#define present(s)	(((s)>>47)&1)
#define l(s)		(((s)>>53)&1)
#define db(s)		(((s)>>54)&1)
#define g(s)		(((s)>>55)&1)

#define extract_segment(sel, sval) { \
		.selector = (sel), \
		.base = base((sval)), \
		.limit = limit((sval)), \
		.type = type((sval)), \
		.s = s((sval)), \
		.dpl = dpl((sval)), \
		.present = present((sval)), \
		.l = l((sval)), \
		.db = db((sval)), \
		.g = g((sval)), \
}
 */

int kvm_handle_syscall(struct vm *vm, struct vcpu *vcpu){
	int vcpufd = vcpu->fd;
	int ret = -1;

	struct kvm_regs regs;
	struct kvm_sregs sregs;
	struct kvm_msrs *msrs = (struct kvm_msrs*)malloc(sizeof(struct kvm_msrs)+sizeof(struct kvm_msr_entry)*3);

	if(!msrs)
		return -1;

	msrs->nmsrs = 3;
	msrs->entries[0].index = MSR_STAR;
	msrs->entries[1].index = MSR_LSTAR;
	msrs->entries[2].index = MSR_SFMASK;

	if(ioctl(vcpufd, KVM_GET_MSRS, msrs) < 0){
		perror("ioctl KVM_GET_MSRS");
		goto err;
	}
	if(ioctl(vcpufd, KVM_GET_REGS, &regs) < 0){
		perror("ioctl KVM_GET_REGS");
		goto err;
	}
	if(ioctl(vcpufd, KVM_GET_SREGS, &sregs) < 0){
		perror("ioctl KVM_GET_SREGS");
		goto err;
	}

	uint16_t kernel_cs = msrs->entries[0].data >> 32;
	uint64_t syscall_handler = msrs->entries[1].data;
	uint64_t flag_mask = msrs->entries[2].data;

	regs.rcx = regs.rip + 2;
	regs.r11 = regs.rflags;
	regs.rip = syscall_handler;
	regs.rflags &= flag_mask;

	/*
	uint64_t *gdt_base = (uint64_t*)translate(vcpufd, sregs.gdt.base);
	if(!gdt_base)
		goto err;

	gdt_base = (uint64_t*)guest2phys(vm, (uint64_t)gdt_base);

	uint64_t cs_raw = gdt_base[kernel_cs/8], ss_raw = gdt_base[kernel_cs/8 + 1];
	struct kvm_segment cs = extract_segment(kernel_cs, cs_raw);
	struct kvm_segment ss = extract_segment(kernel_cs + 8, ss_raw);
	 */
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = kernel_cs,
		.present = 1,
		.type = 11,
		.dpl = 0,
		.db = 0,
		.s = 1,
		.l = 1,
		.g = 1,
	};
	sregs.cs = seg;
	seg.type = 3;
	seg.selector = kernel_cs + 8;
	sregs.ss = seg;

	if(ioctl(vcpufd, KVM_SET_SREGS, &sregs) < 0){
		perror("ioctl KVM_SET_SREGS");
		goto err;
	}

	if(ioctl(vcpufd, KVM_SET_REGS, &regs) < 0){
		perror("ioctl KVM_SET_REGS");
		goto err;
	}

	ret = 0;
err:
	free(msrs);
	return ret;
}
