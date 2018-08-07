#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "hypercall.h"

#define NR_read         0
#define NR_write        1

#define NR_mmap         9

#define NR_munmap       11
#define NR_brk          12

#define NR_exit         60

ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, void *buf, size_t count);
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int sys_munmap(void *addr, size_t length);
int sys_brk(void *addr);
void sys_exit(int status);

uint64_t syscall(uint64_t nr, uint64_t argv[]){
	uint64_t ret = -1;

	switch(nr){
		case NR_read:
			ret = sys_read(argv[0], (void*)argv[1], argv[2]);
			break;
		case NR_write:
			ret = sys_write(argv[0], (void*)argv[1], argv[2]);
			break;
		case NR_mmap:
			ret = (uint64_t)sys_mmap((void*)argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
			break;
		case NR_munmap:
			ret = sys_munmap((void*)argv[0], argv[1]);
			break;
		case NR_brk:
			ret = sys_brk((void*)argv[0]);
			break;
		case NR_exit:
			sys_exit(argv[0]);
			break;
	}

	return ret;
}

ssize_t sys_read(int fd, void *buf, size_t count){
	return hc_read(buf, count, 1);
}

ssize_t sys_write(int fd, void *buf, size_t count){
	return hc_write(buf, count, 1);
}

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
	return hc_malloc(addr, length);
}

int sys_munmap(void *addr, size_t length){
	return hc_free(addr);
}

int sys_brk(void *addr){

}

void sys_exit(int status){
	hlt();
}
