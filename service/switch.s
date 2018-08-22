global set_handler, switch_user, syscall_handler
extern syscall
extern kernel_stack

set_handler:
	xor rax, rax
	mov rdx, 0x00200010
	mov ecx, 0xc0000081
	wrmsr

	mov rax, rdi
	mov rdx, 0x80
	mov ecx, 0xc0000082
	wrmsr

	ret

switch_user:
	cli
	mov  [rel + kernel_stack], rsp
	mov   ax, 0x2b
	mov   ds, eax
	push 0x2b	; ss
	push rsi	; stack
	pushfq		; rflags
	or   qword [rsp], 0x200
	push 0x23	; cs
	push rdi	; rip
	xor rax, rax
	xor rcx, rcx
	xor rdx, rdx
	xor rbx, rbx
	xor rsi, rsi
	xor rdi, rdi
	xor rbp, rbp
	xor r8, r8
	xor r9, r9
	xor r10, r10
	xor r11, r11
	xor r12, r12
	xor r13, r13
	xor r14, r14
	xor r15, r15
	iretq

syscall_handler:
	mov  r12, rsp
	mov  rsp, [rel + kernel_stack]

	push rax
	call get_usercs
	mov  r13, rax
	pop  rax

	push r13	; ss
	push r12	; rsp
	push r11	; rflags
	push r13	; cs
	push rcx	; rip

	add  word [rsp+0x8], 3		; cs
	add  word [rsp+0x20], 0xb	; ss

	push rbx
	push rsi
	push rdi
	push rbp
	mov rbp, rsp

	push r9
	push r8
	push r10
	push rdx
	push rsi
	push rdi
	mov rsi, rsp
	mov rdi, rax

	mov bx, ds
	push bx
	mov bx, ss
	mov ds, bx

	call syscall

	pop bx
	mov ds, bx

	leave
	pop rdi
	pop rsi
	pop rbx
	iretq

get_usercs:
	push rcx
	push rdx
	mov ecx, 0xc0000081
	rdmsr
	shr edx, 0x10
	mov rax, rdx
	pop rdx
	pop rcx
	ret
