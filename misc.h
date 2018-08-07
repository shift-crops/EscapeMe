#ifndef _MISC_H
#define _MISC_H

void set_handler(void* func);
void switch_user(uint64_t rip, uint64_t rsp);
void syscall_handler(void);

#endif
