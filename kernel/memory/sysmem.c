#include <stdint.h>
#include "bits.h"
#include "memory/memory.h"
#include "service/hypercall.h"

/*
0x8000000000 ~ 0xffffffffff : Kernel
  0x8000000000 ~ 0x8000004000 : binary		1:0:0:0-3
  0x8000200000 ~ 0x8000204000 : bss			1:0:1:0-3
  0x8040000000 ~ 0x807ffff000 : straight	1:1:0-511:0-511
  0xffffffc000 ~ 0xfffffff000 : stack		1:511:511:508-511
*/

int init_pagetable(void){
	uint64_t mem_size;
	uint64_t *pml4, *pdpt, *pd, *pt;
	uint64_t bss_phys, cr3;

	mem_size = hc_mem_total();
	asm volatile ("mov %0, cr3" : "=r"(cr3));

	if((uint64_t)(pml4 = hc_malloc(0, 0x1000)) == -1)
		return -1;

	if((uint64_t)(pdpt = hc_malloc(0, 0x1000)) == -1)
		return -1;
	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pdpt;
	pml4[1] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pdpt;

	if((uint64_t)(pd = hc_malloc(0, 0x1000*3)) == -1)
		return -1;
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;

	uint16_t n_pt = (mem_size>>21) + (mem_size & ((1<<21)-1)) ? 1 : 0;
	if((uint64_t)(pt = hc_malloc(0, 0x1000*(2+n_pt+1))) == -1)
		return -1;
	pd[0] = PDE64_PRESENT | PDE64_GLOBAL | (uint64_t)pt;
	for(int i = 0; i < 4; i++)
		pt[i] = PDE64_PRESENT | PDE64_GLOBAL | (i<<12);

	pt += 512;
	if((bss_phys = (uint64_t)hc_malloc(0, 0x1000*4)) == -1)
		return -1;
	pd[1] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pt;
	for(int i = 0; i < 4; i++)
		pt[i] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (bss_phys+(i<<12));

	pd += 512;
	pt += 512;

	pdpt[1] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;
	for(int i = 0; i < n_pt; i++, pt += 512){
		pd[i] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pt;
		uint16_t n_page = (mem_size>>21)-i > 0 ? 512 : (mem_size & ((1<<21)-1))>>12;
		for(int j = 0; j < n_page; j++)
			pt[j] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | ((i<<21)|(j<<12));
	}

	pd += 512;

	pdpt[511] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;
	pd[511] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pt;
	for(int i = 1; i < 5; i++)
		pt[512-i] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (kernel_stack-(i<<12));

	asm volatile (
		"mov rdx, 0x8000000000\r\n"
		"add rdx, [rbp+0x8]\r\n"
		"mov [rbp+0x8], rdx\r\n"

		"mov rdx, 0xfffffff000\r\n"
		"mov rcx, [rbp]\r\n"
		"and rcx, 0xfff\r\n"
		"or rcx, rdx\r\n"
		"mov [rbp], rcx\r\n"

		"and rbp, 0xfff\r\n"
		"add rbp, rdx\r\n"

		"mov cr3, %0\r\n"
	::"r"(pml4));

	return 0;
}

int init_gdt(void){
	#define tsdp(type, s, dpl, p)	(type | (s << 4) | (dpl << 5) | (p << 7))
	#define saldg(slh, avl, l, db, g)	(slh | (avl << 4) | (l << 5) | (db << 6) | (g << 7))

	struct gdtr {
		uint16_t size;
		struct gdt *base __attribute__((packed));
	} gdtr;

	struct gdt {
		uint16_t seg_lim_low;
		uint16_t base_low;
		uint8_t base_mid;
		uint8_t tsdp;
		uint8_t saldg;
		uint8_t base_high;
	} *gdtptr;

	if((uint64_t)(gdtptr = (struct gdt*)(hc_malloc(0, 0x1000)+STRAIGHT_BASE)) == -1)
		return -1;
	gdtr.size = sizeof(struct gdt)*6;
	gdtr.base = gdtptr;

	struct gdt gdt = {
		.seg_lim_low = 0xffff,
		.base_low = 0x0,
		.base_mid = 0x0,
		.tsdp = tsdp(11, 1, 0, 1),
		.saldg = saldg(15, 0, 1, 0, 1),
		.base_high  = 0x0,
	};
	gdtptr[2] = gdt;

	gdt.tsdp = tsdp(3, 1, 0, 1);
	gdtptr[3] = gdt;

	gdt.tsdp = tsdp(11, 1, 3, 1);
	gdtptr[4] = gdt;

	gdt.tsdp = tsdp(3, 1, 3, 1);
	gdtptr[5] = gdt;

	asm volatile (
		"cli\r\n"
		"lgdt [%0]\r\n"
		"sti\r\n"
	::"r"(&gdtr));

	return 0;
}
