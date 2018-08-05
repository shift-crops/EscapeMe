global	set_handler, switch_user, hlt

set_handler:
	mov rax, 0
	mov rdx, 0x100000
	mov ecx, 0xc0000081
	wrmsr
	mov rax, rdi
	mov rdx, rax
	shr rdx, 0x20
	mov ecx, 0xc0000082
	wrmsr
	ret

switch_user:
	cli
	mov   ax, 0x2b
	mov   ds, eax
	push  0x2b
	push  rsi
	pushfq
	or    qword [rsp], 0x200
	push  0x23
	push  rdi
	iretq

hlt:
	hlt
	jmp hlt
