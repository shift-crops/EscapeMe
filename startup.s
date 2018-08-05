global	_start
extern	kernel_main

_start:
	mov rax, 0x21
	mov rbx, 0
	mov rcx, 0x2000
	vmmcall

	mov rsp, rax
	add rsp, 0x2000

	;call init_gdt

	;mov ax, 0x18
	;mov ds, ax
	;mov ss, ax

	;mov rax, 0x10
	;shl rax, 32
	;add rax, kernel_main
	;push rax
	;retf
	call kernel_main

init_gdt:
	cli
	lgdt [gdt_toc]
	sti
	ret

gdt_toc:
	dw 8*6
	dd _gdt	

_gdt:
	; null descriptor
	dw 0x0000
	dw 0x0000
	dw 0x0000		
	dw 0x0000		

	; not in use
	dw 0x0000
	dw 0x0000
	dw 0x0000		
	dw 0x0000		

	; kernel code descriptor				
	dw 0xffff		; Segment Limit Low
	dw 0x0000		; Base Address Low
	db 0x00			; Base Address Mid
	db 10011011b	; type = 11, s = 1, dpl = 0, p = 1 (reverse)
	db 10101111b	; Segment Limit Hi = 15, avl = 0, l = 1, d/b = 0, g = 1 (reverse)
	db 0x00			; Base Address Hi

	; kernel data descriptor				
	dw 0xffff
	dw 0x0000
	db 0x00	
	db 10010011b	; type = 3
	db 10101111b
	db 0x00

	; user code descriptor				
	dw 0xffff
	dw 0x0000
	db 0x00	
	db 11111011b	; type = 11, dpl = 3
	db 10101111b
	db 0x00	

	; user data descriptor				
	dw 0xffff
	dw 0x0000
	db 0x00	
	db 11110011b	; type = 3, dpl = 3
	db 10101111b
	db 0x00

