#include <stdio.h>
#include <stdlib.h>

char *itoa(int64_t value, char *str, int radix){
	int i, n;
	int64_t _v;
	char *p = str;

	if(radix != 10 && radix != 16)
		return NULL;

	if(value<0){
		value *= -1;
		*(p++) = '-';
	}

	for(n=0, _v=value; _v/=radix; n++);
	for(i=n; i>=0; i--, value/=radix)
		p[i] = value%radix + (value%radix < 10 ? '0':('a'-10));
	p[n+1] = '\0';

	return str;
}

