#include <stdio.h>
#include <assert.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>

#define DEBUG_PRINT(...)	fprintf(stderr, __VA_ARGS__)

#define dump_segment_register(n, s) \
	DEBUG_PRINT("%3s base=%016llx limit=%08x selector=%04x type=%02x dpl=%d db=%d l=%d g=%d avl=%d\n", \
		    (n), (s)->base, (s)->limit, (s)->selector, (s)->type, (s)->dpl, (s)->db, (s)->l, (s)->g, (s)->avl )
#define dump_dtable(n, s) \
	DEBUG_PRINT("%3s base=%016llx limit=%04x \n", (n), (s)->base, (s)->limit)

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

	dump_segment_register("cs", &sregs.cs);
	dump_segment_register("ds", &sregs.ds);
	dump_segment_register("es", &sregs.es);
	dump_segment_register("ss", &sregs.ss);
	dump_segment_register("tr", &sregs.tr);

	dump_dtable("gdt", &sregs.gdt);
	dump_dtable("ldt", &sregs.ldt);

	DEBUG_PRINT("cr0=%016llx\n", sregs.cr0);
	DEBUG_PRINT("cr3=%016llx\n", sregs.cr3);
}
