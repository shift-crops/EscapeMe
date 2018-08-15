global memcpy, hlt

memcpy:
    mov rcx, rdx
    l:
    mov al, [rsi]
    mov [rdi], al
    inc rdi
    inc rsi
    loop l
    ret

hlt:
	hlt
	jmp hlt
