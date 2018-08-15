global _start
extern main, exit

_start:
	call main
	mov rdi, rax
	call exit
hlt:
	hlt
	jmp hlt
