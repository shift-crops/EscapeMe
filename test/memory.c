#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int _start(void){
	char *buf1, *buf2;

	puts("malloc 0x100");
	buf1 = malloc(0x100);
	puts("malloc 0x30000");
	buf2 = malloc(0x30000);

	write(1, "Input strings...", 16);
	read(0, buf1, 128);
	puts(buf1);

	write(1, "Input strings...", 16);
	read(0, buf2, 128);
	puts(buf2);

	puts("malloc 0x100");
	free(buf1);
	puts("free 0x30000");
	free(buf2);

	exit(0);

	return 0;
}
