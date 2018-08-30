#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "../syscall.h"

ssize_t write(int fd, void *buf, size_t count){
	ssize_t n;

	if(!count)
	  return 0;

	if (fd < 0 || buf == NULL)
		return -1;

	syscall(n, NR_write);

	return n;
}
