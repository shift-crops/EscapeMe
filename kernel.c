#include <stdint.h>
#include "hypercall.h"
#include "misc.h"

#define STRAIGHT_BASE	0x8040000000

static void init_pagetable(void);
static void init_gdt(void);
static int prepare_user(void);
static void _syscall_handler(void);

void kernel_main(void){
	init_pagetable();
	// --- new paging enabled ---
	init_gdt();

	set_handler(_syscall_handler);
	if(prepare_user() < 0)
		goto hlt;

	switch_user(0x000040017c, 0x7ffffffff0);

hlt:
	asm("hlt");
	goto hlt;
}

#define PDE64_PRESENT	1
#define PDE64_RW		(1U << 1)
#define PDE64_USER		(1U << 2)
#define PDE64_ACCESSED	(1U << 5)
#define PDE64_DIRTY		(1U << 6)
#define PDE64_PS		(1U << 7)
#define PDE64_GLOBAL	(1 << 8)
#define PDE64_NX		(0UL << 63)

static void init_pagetable(void){
	uint64_t mem_size;
	uint64_t *pml4, *pdpt, *pd, *pt;
	uint64_t sp, cr3;

	mem_size = hc_mem_total();
	asm volatile (
		"mov %0, rsp\r\n"
		"mov %1, cr3"
	:"=r"(sp), "=r"(cr3));
	sp = sp & ~0xfff;

	pml4 = hc_malloc(0, 0x1000);

	pdpt = hc_malloc(0, 0x1000);
	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pdpt;
	pml4[1] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pdpt;

	pd = hc_malloc(0, 0x1000*3);
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;

	uint16_t n_pt = (mem_size>>21) + 1;
	pt = hc_malloc(0, 0x1000*(1+n_pt+1));
	pd[0] = PDE64_PRESENT | PDE64_GLOBAL | (uint64_t)pt;
	for(int i = 0; i < 8; i++)
		pt[i] = PDE64_PRESENT | PDE64_GLOBAL | (i<<12);

	pd += 512;
	pt += 512;

	pdpt[1] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;
	for(int i = 0; i < n_pt; i++, pt += 512){
		pd[i] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pt;
		for(int j = 0; j < ((mem_size-(i<<21))>>12); j++)
			pt[j] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | ((i<<21)|(j<<12));
	}

	pd += 512;

	pdpt[511] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pd;
	pd[511] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (uint64_t)pt;
	for(int i = 0; i < 2; i++)
		pt[511-i] = PDE64_PRESENT | PDE64_RW | PDE64_GLOBAL | (sp-(i<<12));

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
}

static void init_gdt(void){
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

	gdtptr = (struct gdt*)(hc_malloc(0, 0x1000)+STRAIGHT_BASE);
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
}

static int prepare_user(){
	uint64_t pml4_phys, pdpt_phys, pd_phys, pt_phys, page_phys;
	uint64_t *pml4, *pdpt, *pd, *pt;

	asm volatile ("mov %0, cr3":"=r"(pml4_phys));
	pml4 = (uint64_t*)(pml4_phys+STRAIGHT_BASE);

	if((pdpt_phys = (uint64_t)hc_malloc(0, 0x1000)) == -1)
		return -1;
	pdpt = (uint64_t*)(pdpt_phys+STRAIGHT_BASE);
	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_phys;

	if((pd_phys = (uint64_t)hc_malloc(0, 0x1000*2)) == -1)
		return -1;
	pd = (uint64_t*)(pd_phys+STRAIGHT_BASE);
	pdpt[0] = PDE64_PRESENT | PDE64_USER | pd_phys;
	pdpt[511] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pd_phys+0x1000);

	if((pt_phys = (uint64_t)hc_malloc(0, 0x1000*2)) == -1)
		return -1;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	pd[2] = PDE64_PRESENT | PDE64_USER | pt_phys;
	pd += 512;
	pd[511] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pt_phys+0x1000);

	if((page_phys = (uint64_t)hc_load_user(0, 0x1000*2)) == -1)
		return -1;
	for(int i = 0; i < 2; i++)
		pt[i] = PDE64_PRESENT | PDE64_USER | (page_phys+0x1000*i);
	pt += 512;

	if((page_phys = (uint64_t)hc_malloc(0, 0x1000*4)) == -1)
		return -1;
	for(int i = 512-4; i < 512; i++)
		pt[i] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (page_phys+0x1000*i);

	return 0;
}

static void _syscall_handler(void){
	asm("leave\r\njmp syscall_handler");
}
