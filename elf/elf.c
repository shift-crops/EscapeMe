#include <elf.h>
#include <sys/mman.h>
#include "elf/elf.h"
#include "service/hypercall.h"
#include "memory/memory.h"
#include "memory/usermem.h"

uint64_t load_elf(void){
	uint64_t ehdr_phys;
	uint64_t ret = -1;

	if((ehdr_phys = (uint64_t)hc_load_module(0, 0, 0x1000)) == -1)
		return -1;
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)(ehdr_phys+STRAIGHT_BASE);

	if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0 && ehdr->e_ident[EI_MAG1] == ELFMAG1 && \
				ehdr->e_ident[EI_MAG2] == ELFMAG2 && ehdr->e_ident[EI_MAG3] == ELFMAG3 ))
		goto end;

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
		goto end;

	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
		goto end;

	for(int i = 0; i < ehdr->e_phnum; i++) {
		Elf64_Phdr *phdr = (Elf64_Phdr *)((uint64_t)ehdr + ehdr->e_phoff + ehdr->e_phentsize * i);
		uint64_t entry_phys;

		if(phdr->p_type != PT_LOAD)
			continue;

		if((entry_phys = (uint64_t)hc_load_module(0, phdr->p_offset, phdr->p_filesz)) == -1)
			goto end;

		uint16_t flags = phdr->p_flags;
		uint64_t memsz = (phdr->p_memsz + 0x1000-1) & ~0xfff;
		mmap_in_user(phdr->p_vaddr, entry_phys, memsz, \
				(flags & PF_R ? PROT_READ : 0) | (flags & PF_W ? PROT_WRITE : 0) | (flags & PF_X ? PROT_EXEC : 0));

		if(phdr->p_vaddr + memsz > brk)
			brk = phdr->p_vaddr + memsz;
	}

	ret = ehdr->e_entry;
end:
	hc_free((void*)ehdr_phys, 0x1000);
	return ret;
}
