#include <stdint.h>

uint64_t syscall(uint64_t nr, uint64_t argv[]){
	if(nr == 60)
		asm("hlt");
	return 0xdeadbeef;
}
