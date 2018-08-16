#include <stdio.h>

char *itoa(int value, char *str, int radix){
	int i, n, _v;

	if(radix != 10 && radix != 16)
		return NULL;

	if(value<0){
		value *= -1;
		*(str++) = '-';
	}

	for(n=0, _v=value; _v/=radix; n++);
	for(i=n; i>=0; i--, value/=radix)
		str[i] = value%radix + (value%radix < 10 ? '0':('a'-10));
	str[n+1] = '\0';

	return str;
}

