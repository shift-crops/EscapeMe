#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void _start(void){
	char buf[128];

	read(0, buf, 128);
	puts(buf);

	exit(0);
}
