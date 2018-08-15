global hc_read, hc_write
global hc_mem_inuse, hc_mem_total, hc_malloc, hc_free
global hc_load_module

hc_read:
	mov rax, 0x10
	mov rbx, rdi
	mov rcx, rsi
	vmmcall
	ret

hc_write:
	mov rax, 0x11
	mov rbx, rdi
	mov rcx, rsi
	vmmcall
	ret

hc_mem_total:
	mov rax, 0x20
	mov rbx, 1
	vmmcall
	ret

hc_mem_inuse:
	mov rax, 0x20
	mov rbx, 2
	vmmcall
	ret

hc_malloc:
	mov rax, 0x21
	mov rbx, rdi
	mov rcx, rsi
	vmmcall
	ret

hc_free:
	mov rax, 0x22
	mov rbx, rdi
	mov rcx, rsi
	vmmcall
	ret

hc_load_module:
	mov rax, 0x30
	mov rbx, rdi
	mov rcx, rsi
	vmmcall
	ret
