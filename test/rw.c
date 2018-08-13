#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
	char buf[128];

	read(0, buf, 128);
	puts(buf);

	return 0;
}
