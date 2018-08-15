#include <stdint.h>
#include "bits.h"
#include "elf/elf.h"
#include "memory/sysmem.h"
#include "memory/usermem.h"
#include "service/switch.h"

static void _syscall_handler(void);

void kernel_main(void){
	uint64_t entry;

	init_pagetable();
	// --- new paging enabled ---
	init_gdt();

	set_handler(_syscall_handler);
	if(prepare_user() < 0)
		goto hlt;

	if((entry = load_elf()) == -1)
		goto hlt;

	switch_user(entry, 0x7ffffffff0);

hlt:
	asm("hlt");
	goto hlt;
}

static void _syscall_handler(void){
	asm("leave\r\njmp syscall_handler");
}
