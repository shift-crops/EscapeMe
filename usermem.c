#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "kernel.h"
#include "bits.h"
#include "hypercall.h"

/*
0x0000000000 ~ 0x7fffffffff : User
  0x0000400000 ~ 0x0000403000 : binary		0:0:2:0-2
  0x0000603000 ~ 0x0000604000 : bss			0:0:3:3
  0x0000608000 ~ 0x0000610000 : heap 		0:0:3:8-16
  0x7fffff0000 ~ 0x7ffffff000 : stack		0:511:511:496-511
*/

int prepare_user(void){
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
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_phys;
	pdpt[511] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pd_phys+0x1000);

	if((pt_phys = (uint64_t)hc_malloc(0, 0x1000*3)) == -1)
		return -1;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	pd[2] = PDE64_PRESENT | PDE64_USER | pt_phys;
	pd[3] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pt_phys+0x1000);

	pd += 512;
	pd[511] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (pt_phys+0x2000);

	// binary
	if((page_phys = (uint64_t)hc_load_user(0, 0x1000*3)) == -1)
		return -1;
	for(int i = 0; i < 3; i++)
		pt[i] = PDE64_PRESENT | PDE64_USER | (page_phys+0x1000*i);

	// bss
	pt += 512;
	if((page_phys = (uint64_t)hc_malloc(0, 0x1000*2)) == -1)
		return -1;
	for(int i = 0; i < 2; i++)
		pt[3+i] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (page_phys+0x1000*i);

	// stack
	pt += 512;
	if((page_phys = (uint64_t)hc_malloc(0, 0x1000*4)) == -1)
		return -1;
	for(int i = 512-4; i < 512; i++)
		pt[i] = PDE64_PRESENT | PDE64_RW | PDE64_USER | (page_phys+(i<<12));

	return 0;
}

uint64_t mmap_user(uint64_t addr, size_t length, int prot){
	uint64_t pml4_phys, pdpt_phys, pd_phys, pt_phys, page_phys;
	uint64_t *pml4, *pdpt, *pd, *pt;
	static uint64_t map_bottom;

	if(addr&0xfff || length&0xfff)
		return -1;

	if(!map_bottom)
		map_bottom = 0x7fff200000;

	if(!addr)
		addr = map_bottom - length;

	uint16_t idx[]  = { (addr>>39) & 0x1ff, (addr>>30) & 0x1ff, (addr>>21) & 0x1ff, (addr>>12) & 0x1ff };
	uint64_t ret = -1;

	uint16_t pages  = length>>12;
	uint16_t remain = 0;
	if(pages > 512-idx[3]){
		remain = pages - (512-idx[3]);
		pages = 512-idx[3];
	}

	asm volatile ("mov %0, cr3":"=r"(pml4_phys));
	pml4 = (uint64_t*)(pml4_phys+STRAIGHT_BASE);
	if(!(pml4[idx[0]] & PDE64_PRESENT) || !(pml4[idx[0]] & PDE64_USER)){
		ret= -1;									// user_page must in 0x0000000000 ~ 0x7fffffffff
		goto end;
	}

	pdpt_phys = pml4[idx[0]] & ~0xfff;
	pdpt = (uint64_t*)(pdpt_phys+STRAIGHT_BASE);
	if(!(pdpt[idx[1]] & PDE64_PRESENT))
		goto new_pd;
	if(!(pdpt[idx[1]] & PDE64_USER)){
		ret= -1;
		goto end;
	}

	pd_phys = pdpt[idx[1]] & ~0xfff;
	pd = (uint64_t*)(pd_phys+STRAIGHT_BASE);
	if(!(pd[idx[2]] & PDE64_PRESENT))
		goto new_pt;
	if(!(pd[idx[2]] & PDE64_USER)){
		ret= -1;
		goto end;
	}

	pt_phys = pd[idx[2]] & ~0xfff;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	for(int i = 0; i < pages; i++)
		if(pt[idx[3]+i] & PDE64_PRESENT){
			// sudeni aru yanke!w
			if(addr == map_bottom - length){
				map_bottom -= (i+1)<<12;
				ret= -2;
			}
			else
				ret= -1;
			goto end;
		}
	goto new_page;


new_pd:
	if((pd_phys = (uint64_t)hc_malloc(0, 0x1000)) == -1)
		return -1;
	pd = (uint64_t*)(pd_phys+STRAIGHT_BASE);
	pdpt[idx[1]] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_phys;

new_pt:
	if((pt_phys = (uint64_t)hc_malloc(0, 0x1000)) == -1)
		return -1;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	pd[idx[2]] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pt_phys;

new_page:
	if((page_phys = (uint64_t)hc_malloc(0, pages<<12)) == -1)
		return -1;
	for(int i = 0; i < pages; i++)
		pt[idx[3]+i] = PDE64_PRESENT | (prot & PROT_WRITE ? PDE64_RW : 0) | \
					   (prot & (PROT_READ | PROT_WRITE) ? PDE64_USER : 0) | (page_phys+(i<<12));

	if(remain > 0)
		if(mmap_user(addr+(pages<<12), remain<<12, prot) < 0)
			return -1;

	if(addr == map_bottom - length)
		map_bottom -= length;

	ret = addr;
end:
	if(ret == -2)
		ret = mmap_user(0, length, prot);
	return ret;
}

uint64_t mprotect_user(uint64_t addr, size_t length, int prot){
	uint16_t idx[]  = { (addr>>39) & 0x1ff, (addr>>30) & 0x1ff, (addr>>21) & 0x1ff, (addr>>12) & 0x1ff };
	uint64_t pml4_phys, pdpt_phys, pd_phys, pt_phys;
	uint64_t *pml4, *pdpt, *pd, *pt;

	if(addr&0xfff || length&0xfff)
		return -1;

	uint16_t pages  = length>>12;
	uint16_t remain = 0;
	if(pages > 512-idx[3]){
		remain = pages - (512-idx[3]);
		pages = 512-idx[3];
	}

	asm volatile ("mov %0, cr3":"=r"(pml4_phys));
	pml4 = (uint64_t*)(pml4_phys+STRAIGHT_BASE);
	if(!(pml4[idx[0]] & PDE64_PRESENT) || !(pml4[idx[0]] & PDE64_USER))
		return -1;

	pdpt_phys = pml4[idx[0]] & ~0xfff;
	pdpt = (uint64_t*)(pdpt_phys+STRAIGHT_BASE);
	if(!(pdpt[idx[1]] & PDE64_PRESENT) || !(pdpt[idx[1]] & PDE64_USER))
		return -1;

	pd_phys = pdpt[idx[1]] & ~0xfff;
	pd = (uint64_t*)(pd_phys+STRAIGHT_BASE);
	if(!(pd[idx[2]] & PDE64_PRESENT) || !(pd[idx[2]] & PDE64_USER))
		return -1;

	pt_phys = pd[idx[2]] & ~0xfff;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	for(int i = 0; i < pages; i++)
		if(!(pt[idx[3]+i] & PDE64_PRESENT))
			return -1;

	for(int i = 0; i < pages; i++){
		uint64_t page_phys = pt[idx[3]+i] & ~((1<<9)-1);

		pt[idx[3]+i] = PDE64_PRESENT | (prot & PROT_WRITE ? PDE64_RW : 0) | \
					   (prot & (PROT_READ | PROT_WRITE) ? PDE64_USER : 0) | page_phys;
	}

	if(remain > 0)
		if(mprotect_user(addr+(pages<<12), remain<<12, prot) < 0)
			return -1;

	return 0;
}

uint64_t munmap_user(uint64_t addr, size_t length){
	uint16_t idx[]  = { (addr>>39) & 0x1ff, (addr>>30) & 0x1ff, (addr>>21) & 0x1ff, (addr>>12) & 0x1ff };
	uint64_t pml4_phys, pdpt_phys, pd_phys, pt_phys;
	uint64_t *pml4, *pdpt, *pd, *pt;

	if(addr&0xfff || length&0xfff)
		return -1;

	uint16_t pages  = length>>12;
	uint16_t remain = 0;
	if(pages > 512-idx[3]){
		remain = pages - (512-idx[3]);
		pages = 512-idx[3];
	}

	asm volatile ("mov %0, cr3":"=r"(pml4_phys));
	pml4 = (uint64_t*)(pml4_phys+STRAIGHT_BASE);
	if(!(pml4[idx[0]] & PDE64_PRESENT) || !(pml4[idx[0]] & PDE64_USER))
		return -1;

	pdpt_phys = pml4[idx[0]] & ~0xfff;
	pdpt = (uint64_t*)(pdpt_phys+STRAIGHT_BASE);
	if(!(pdpt[idx[1]] & PDE64_PRESENT) || !(pdpt[idx[1]] & PDE64_USER))
		return -1;

	pd_phys = pdpt[idx[1]] & ~0xfff;
	pd = (uint64_t*)(pd_phys+STRAIGHT_BASE);
	if(!(pd[idx[2]] & PDE64_PRESENT) || !(pd[idx[2]] & PDE64_USER))
		return -1;

	pt_phys = pd[idx[2]] & ~0xfff;
	pt = (uint64_t*)(pt_phys+STRAIGHT_BASE);
	for(int i = 0; i < pages; i++)
		if(!(pt[idx[3]+i] & PDE64_PRESENT))
			return -1;

	for(int i = 0; i < pages; i++){
		uint64_t page_phys = pt[idx[3]+i] & ~((1<<9)-1);
		hc_free((void*)page_phys, 0x1000);

		pt[idx[3]+i] = 0;
	}

	if(remain > 0)
		if(munmap_user(addr+(pages<<12), remain<<12) < 0)
			return -1;

	return 0;
}
