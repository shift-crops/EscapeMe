#include <stdlib.h>
#include <unistd.h>

int _start(void){
	char *buf1, *buf2;

	buf1 = malloc(0x100);
	buf2 = malloc(0x30000);

	read(0, buf1, 128);
	write(1, buf1, 128);
	read(0, buf2, 128);
	write(1, buf2, 128);

	free(buf1);
	free(buf2);

	return 0;
}
