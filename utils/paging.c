#include <stdint.h>
#include "vm/vm.h"
#include "vm/bits.h"

uint64_t paging(struct vm *vm, uint64_t pml4_addr, uint64_t addr){
	uint16_t idx[]  = { (addr>>39) & 0x1ff, (addr>>30) & 0x1ff, (addr>>21) & 0x1ff, (addr>>12) & 0x1ff };

	uint64_t *pml4	= guest2phys(vm, pml4_addr);

	uint64_t pdpt_addr 	= pml4[idx[0]] & ~0xfff;
	uint64_t *pdpt	= guest2phys(vm, pdpt_addr);

	uint64_t pd_addr 	= pdpt[idx[1]] & ~0xfff;
	uint64_t *pd 	= guest2phys(vm, pd_addr);

	uint64_t pt_addr 	= pd[idx[2]] & ~0xfff;
	if(pd[idx[2]] & PDE64_PS)
		return pt_addr | (addr&0x1fffff);

	uint64_t *pt 	= guest2phys(vm, pt_addr);
	return (pt[idx[3]] & ~0xfff) | (addr&0xfff);
}
