global	_start
extern	kernel_main

_start:
	mov rax, 0x21
	mov rbx, 0
	mov rcx, 0x2000
	vmmcall

	mov rsp, rax
	add rsp, 0x2000
	call kernel_main
