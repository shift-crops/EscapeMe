#include <stdio.h>
#include <unistd.h>

int _start(void){
	char buf[128];

	read(0, buf, 128);
	puts(buf);

	return 0;
}
