#include <stddef.h>
#include <stdio.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
	asm volatile (
		"mov r10, rcx\r\n"
		"mov rax, 0x09\r\n"
		"syscall"
		);

}

int mprotect(void *addr, size_t len, int prot){
	asm volatile (
		"mov rax, 0x0a\r\n"
		"syscall"
		);
}

int munmap(void *addr, size_t length){
	asm volatile (
		"mov rax, 0x0b\r\n"
		"syscall"
		);
}
