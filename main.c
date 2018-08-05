#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "vm/vm.h"

#define GUEST_MEMSIZE	0x800000

__attribute__((constructor))
void init(void){
	setbuf(stdout, NULL);
}

int main(int argc, char *argv[]){
	struct vm *vm;
	int fd;
	unsigned long entry;
	char *image_file;

	image_file = argc > 1 ? argv[1] : "kernel.bin";

	if((fd = open(image_file, O_RDONLY)) < 0){
		perror(image_file);
		return -1;
	}

	if(!(vm = init_vm(1, GUEST_MEMSIZE)))
		return -1;

	if((entry = load_image(vm, fd)) & 0xfff)
		return -1;

	close(fd);

	run_vm(vm, 0, entry);

	return 0;
}

