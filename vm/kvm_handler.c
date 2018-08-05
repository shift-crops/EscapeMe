#include <unistd.h>
#include <linux/kvm.h>
#include "kvm_handler.h"
#include "vm.h"
#include "utils/gmalloc.h"

void kvm_handle_io(struct vm *vm, struct kvm_regs *regs){

}

void kvm_handle_hypercall(struct vm *vm, struct kvm_regs *regs){
	unsigned nr = regs->rax;
	unsigned long arg[] = {regs->rbx, regs->rcx, regs->rdx, regs->rsi};
	unsigned long ret = -1;

	switch(nr){
		case 0x10:		// read(0, buf, size)
			if(check_addr(vm, arg[0]))
				ret = read(STDIN_FILENO, guest2phys(vm, arg[0]), arg[1]);
			break;
		case 0x11:		// write(1, buf, size)
			if(check_addr(vm, arg[0]))
				ret = write(STDOUT_FILENO, guest2phys(vm, arg[0]), arg[1]);
			break;
		case 0x20:
			ret = get_gmem_remain();
			break;
		case 0x21:		// gmalloc(addr, size)
			ret = gmalloc(arg[0], arg[1]);
			break;
		case 0x22:		// gfree(addr);
			ret = gfree(arg[0]);
			break;
	}

	regs->rax = ret;
}
