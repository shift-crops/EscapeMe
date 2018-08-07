#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "vm/vm.h"
#include "utils/module.h"

#define GUEST_MEMSIZE	0x800000

__attribute__((constructor))
void init(void){
	setbuf(stdout, NULL);
}

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

	run_vm(vm, 0, entry);

	return 0;
}

