#ifndef _HYPERCALL_H
#define _HYPERCALL_H

unsigned int hc_read(void *buf, unsigned long size, int user);
unsigned int hc_write(void *buf, unsigned long size, int user);

unsigned int hc_mem_inuse(void);
unsigned int hc_mem_total(void);
void *hc_malloc(void *addr, unsigned long size);
int hc_free(void *addr, unsigned long size);

void *hc_load_user(void *addr, unsigned long size);

#endif
