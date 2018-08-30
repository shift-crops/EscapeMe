#ifndef _MISC_H
#define _MISC_H

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, const int c, size_t n);
size_t strlen(const char *s);
void hlt(void) __attribute__((noreturn)) ;

#endif
