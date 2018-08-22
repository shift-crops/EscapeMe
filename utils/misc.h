#ifndef _MISC_H
#define _MISC_H

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, const int c, size_t n);
void hlt(void) __attribute__((noreturn)) ;

#endif
