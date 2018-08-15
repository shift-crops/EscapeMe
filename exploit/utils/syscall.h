#define str(s) 		#s

#define syscall_woret(nr)	asm volatile("mov rax, " str(nr) "\r\nsyscall")
#define syscall(x, nr)		asm volatile("mov rax, " str(nr) "\r\nsyscall":"=A"(x))
#define syscall1(x, nr, arg1) \
	do { asm volatile("mov rdi, %0"::"A"((long)arg1)); syscall(x, nr); } while(0)
#define syscall2(x, nr, arg1, arg2) \
	do { asm volatile("mov rsi, %0"::"A"((long)arg2)); syscall1(x, nr, arg1); } while(0)
#define syscall3(x, nr, arg1, arg2, arg3) \
	do { asm volatile("mov rdx, %0"::"A"((long)arg3)); syscall2(x, nr, arg1, arg2); } while(0)
#define syscall4(x, nr, arg1, arg2, arg3, arg4) \
	do { asm volatile("mov r10, %0"::"A"((long)arg4)); syscall3(x, nr, arg1, arg2, arg3); } while(0)
#define syscall5(x, nr, arg1, arg2, arg3, arg4, arg5) \
	do { asm volatile("mov r8, %0"::"A"((long)arg5)); syscall4(x, nr, arg1, arg2, arg3, arg4); } while(0)
#define syscall6(x, nr, arg1, arg2, arg3, arg4, arg5, arg6) \
	do { asm volatile("mov r9, %0"::"A"((long)arg6)); syscall5(x, nr, arg1, arg2, arg3, arg4, arg5); } while(0)


#define NR_read		0
#define NR_write	1

#define NR_mmap		9
#define NR_mprotect	10
#define NR_munmap	11
#define NR_brk		12

#define NR_exit		60
