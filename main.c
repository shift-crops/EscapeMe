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

static int set_seccomp(int maxfd);

int main(int argc, char *argv[]){
	struct vm *vm;
	unsigned long entry;
	int nmod;
	char **mods;

	if(argc < 2){
		char *arg[] = {"kernel.bin"};
		nmod = 1;
		mods = arg;
	}
	else {
		nmod = argc-1;
		mods = argv+1;
	}
	init_modules(nmod, mods);

	if(!(vm = init_vm(1, GUEST_MEMSIZE)))
		return -1;
	if((entry = load_kernel(vm)) & 0xfff)
		return -1;

	if(set_seccomp(3+nmod+3))
		return -1;

	run_vm(vm, 0, entry);

	fini_modules();
	return 0;
}

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/kvm.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

static int set_seccomp(int maxfd){
	/*
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

		BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, __X32_SYSCALL_BIT, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ioctl, 0, 4),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[1]))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_CREATE_VM, 1, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_CREATE_VCPU, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
	};
	 */

	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

		BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, __X32_SYSCALL_BIT, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_read, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_write, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_close, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_lseek, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_brk, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit_group, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ioctl, 0, 1),  // vuln
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[1]))),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_CREATE_VM, 5, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, KVM_CREATE_VCPU, 4, 0),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[0]))),
		BPF_STMT(BPF_ALU | BPF_AND | BPF_K, 0xff),
		BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, maxfd, 1, 0),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
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
