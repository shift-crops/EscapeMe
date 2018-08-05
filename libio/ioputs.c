#include <string.h>
#include <unistd.h>

int puts(const char *s){
	int n;

	if((n = write(1, s, strlen(s))))
		write(1, "\n", 1);

	return n;
}

