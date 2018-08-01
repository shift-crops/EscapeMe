#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

ssize_t write(int fd, void *buf, size_t count){
	if(!count)
	  return 0;

	if (fd < 0 || buf == NULL)
		return -1;

	asm volatile(
		"mov rax, 0x01\r\n"
		"syscall"
	);
	return;
}
