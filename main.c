#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "vm/vm.h"
#include "utils/module.h"

#define GUEST_MEMSIZE	0x400000

__attribute__((constructor))
void init(void){
	setbuf(stdout, NULL);
}

static int set_seccomp(void);

int main(int argc, char *argv[]){
	struct vm *vm;
	unsigned long entry;

	if(argc < 2){
		char *arg[] = {NULL, "kernel.bin"};
		argc = 2;
		argv = arg;
	}
	init_modules(argc-1, argv+1);

	if(!(vm = init_vm(1, GUEST_MEMSIZE)))
		return -1;

	if((entry = load_kernel(vm)) & 0xfff)
		return -1;

	if(set_seccomp())
		return -1;

	run_vm(vm, 0, entry);

	return 0;
}

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/kvm.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

static int set_seccomp(void){
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

		BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, __X32_SYSCALL_BIT, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ioctl, 0, 9),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[1]))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_RUN, 7, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_GET_REGS, 6, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_SET_REGS, 5, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_GET_SREGS, 4, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_SET_SREGS, 3, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_GET_MSRS, 2, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_SET_GUEST_DEBUG, 1, 0),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
	};

	struct sock_fprog prog = {
		.len = (unsigned short) (sizeof(filter) / sizeof(struct sock_filter)),
		.filter = filter,
	};

	if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
		perror("prctl PR_SET_NO_NEW_PRIVS");
		return -1;
	}

	if(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)){
		perror("prctl PR_SET_SECCOMP");
		return -1;
	}

	return 0;
}
