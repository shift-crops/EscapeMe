#include <stddef.h>
#include <stdio.h>
#include "../syscall.h"

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
	void *mem;

	syscall6(mem, NR_mmap, addr, length, prot, flags, fd, offset);

	return mem;
}

int mprotect(void *addr, size_t len, int prot){
	long res;

	syscall(res, NR_mprotect);

	return res;
}

int munmap(void *addr, size_t length){
	long res;

	syscall(res, NR_munmap);

	return res;
}
