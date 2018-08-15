#include <stdint.h>
#include "bits.h"
#include "memory/sysmem.h"
#include "memory/usermem.h"
#include "service/switch.h"

static void _syscall_handler(void);

void kernel_main(void){
	init_pagetable();
	// --- new paging enabled ---
	init_gdt();

	set_handler(_syscall_handler);
	if(prepare_user() < 0)
		goto hlt;

	switch_user(0x0000400180, 0x7ffffffff0);

hlt:
	asm("hlt");
	goto hlt;
}

static void _syscall_handler(void){
	asm("leave\r\njmp syscall_handler");
}
