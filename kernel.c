#include <stdint.h>
#include "bits.h"
#include "elf/elf.h"
#include "memory/sysmem.h"
#include "memory/usermem.h"
#include "service/switch.h"

int kernel_main(void){
	uint64_t entry;

	if((init_pagetable()) < 0)
		return -1;
	// --- new paging enabled ---
	if(init_gdt() < 0)
		return -1;

	asm("lea rdi, [rip + syscall_handler]\n"
		"call set_handler");
	if(prepare_user() < 0)
		return -1;

	if((entry = load_elf()) == -1)
		return -1;

	switch_user(entry, 0x7ffffffff0);

	return 0;
}
