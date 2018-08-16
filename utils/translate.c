#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/kvm.h>
#include "vm/vm.h"
#include "bits.h"

/*
uint64_t translate(int vcpufd, uint64_t addr, int writeable, int user){
	struct kvm_translation trans;

	trans.linear_address = addr;
	trans.usermode = 3;
	if(ioctl(vcpufd, KVM_TRANSLATE, &trans)){
		perror("ioctl KVM_TRANSLATE");
		return 0;
	}

	printf("la %p -> pa:%p, v:%d w:%x, u:%x\n", addr, trans.physical_address, trans.valid, trans.writeable, trans.usermode);

	if(!trans.valid)
		return 0;

	//if(writeable && !trans.writeable)
	//	return 0;

	//if(user && !trans.usermode)
	//	return 0;

	return trans.physical_address;
}
 */

#define CHK_PERMISSION(entry)	(((entry) & PDE64_PRESENT) && \
								(!user || (entry) & PDE64_USER) && \
								(!write || (entry) & PDE64_RW))
uint64_t translate(struct vm *vm, uint64_t pml4_addr, uint64_t laddr, int write, int user){
	uint16_t idx[]  = { (laddr>>39) & 0x1ff, (laddr>>30) & 0x1ff, (laddr>>21) & 0x1ff, (laddr>>12) & 0x1ff };
	uint64_t paddr = -1;

	uint64_t *pml4	= guest2phys(vm, pml4_addr);

	uint64_t pdpt_addr 	= pml4[idx[0]] & ~0xfff;
	if(!CHK_PERMISSION(pml4[idx[0]]))
		goto end;
	uint64_t *pdpt	= guest2phys(vm, pdpt_addr);

	uint64_t pd_addr 	= pdpt[idx[1]] & ~0xfff;
	if(!CHK_PERMISSION(pdpt[idx[1]]))
		goto end;
	uint64_t *pd 	= guest2phys(vm, pd_addr);

	uint64_t pt_addr 	= pd[idx[2]] & ~0xfff;
	if(!CHK_PERMISSION(pd[idx[2]]))
		goto end;
	if(pd[idx[2]] & PDE64_PS){
		if(pd[idx[2]] & PDE64_USER)	// user does not support hugepage
			goto end;

		paddr = pt_addr | (laddr&0x1fffff);
		goto end;	// vuln
	}

	uint64_t *pt 	= guest2phys(vm, pt_addr);
	if(!CHK_PERMISSION(pt[idx[3]]))
		goto end;

	paddr = (pt[idx[3]] & ~0xfff) | (laddr&0xfff);
	assert_addr(vm, paddr);
end:
	//printf("laddr:%p -> paddr:%p\n", laddr, paddr);
	return paddr;
}
