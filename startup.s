global	_start, hlt
extern	kernel_main, kernel_stack

_start:
	mov rax, 0x21
	mov rbx, 0
	mov rcx, 0x2000
	vmmcall

	mov rsp, rax
	add rsp, 0x2000
	mov  [rel + kernel_stack], rsp
	call kernel_main

hlt:
	hlt
	jmp hlt
