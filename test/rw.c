#include <unistd.h>

int _start(void){
	char buf[128];

	read(0, buf, 128);
	write(1, buf, 128);

	return 0;
}
