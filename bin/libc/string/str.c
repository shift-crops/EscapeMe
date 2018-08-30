#include <stddef.h>
#include <stdlib.h>

size_t strlen(const char *s){
	size_t i;
	for(i=0; s[i]; i++);
	return i;
}

char *strcat(char *dest, const char *src){
	char *p;

	for(p=dest; *p; p++);
	for(; *src; p++, src++)
		*p = *src;
	*p = '\0';

	return dest;
}

char *strncat(char *dest, const char *src, size_t n){
	char *p;
	unsigned i;

	for(p=dest; *p; p++);
	for(i=0; i<n && *src; i++, src++)
		p[i] = *src;
	p[i] = '\0';

	return dest;
}

const char *strchr(const char *s, int c){
	for(; *s && (*s ^ (char)c); s++);
	return *s ? s : NULL;
}

char *strdup(const char *s){
	int i;
	char *p;

	p = malloc(strlen(s)+1);
	for(i=0; *s; i++, s++)
		p[i] = *s;
	p[i] = '\0';

	return p;
}
