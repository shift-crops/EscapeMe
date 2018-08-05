#include <stdio.h>
#include <assert.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>

#define DEBUG_PRINT(...)	fprintf(stderr, __VA_ARGS__)

void dump_segment_register(struct kvm_segment *s){
	DEBUG_PRINT("base=%016llx limit=%08x selector=%04x type=%02x\n",
		    s->base, s->limit, s->selector, s->type);
}

void dump_regs(int vcpufd){
	int r;

	struct kvm_regs regs;
	r = ioctl(vcpufd, KVM_GET_REGS, &regs);
	assert(r != -1);

	DEBUG_PRINT("rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx\n",
			regs.rax, regs.rbx, regs.rcx, regs.rdx);
	DEBUG_PRINT("rsi=%016llx rdi=%016llx rsp=%016llx rbp=%016llx\n",
			regs.rsi, regs.rdi, regs.rsp, regs.rbp);
	DEBUG_PRINT("r8 =%016llx r9 =%016llx r10=%016llx r11=%016llx\n",
			regs.r8, regs.r9, regs.r10, regs.r11);
	DEBUG_PRINT("r12=%016llx r13=%016llx r14=%016llx r15=%016llx\n",
			regs.r12, regs.r13, regs.r14, regs.r15);
	DEBUG_PRINT("rip=%016llx rflags=%016llx\n", regs.rip, regs.rflags);

	struct kvm_sregs sregs;
	r = ioctl(vcpufd, KVM_GET_SREGS, &sregs);
	assert(r != -1);

	DEBUG_PRINT("cs ");
	dump_segment_register(&sregs.cs);
	DEBUG_PRINT("ds ");
	dump_segment_register(&sregs.ds);
	DEBUG_PRINT("es ");
	dump_segment_register(&sregs.es);
	DEBUG_PRINT("ss ");
	dump_segment_register(&sregs.ss);
	DEBUG_PRINT("tr ");
	dump_segment_register(&sregs.tr);
	DEBUG_PRINT("cr0=%016llx\n", sregs.cr0);
}
