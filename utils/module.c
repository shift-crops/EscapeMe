#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "vm/vm.h"
#include "utils/palloc.h"
#include "utils/module.h"

struct modules {
	unsigned nmod;
	int fds[];
} *mod_list;

int init_modules(unsigned nmod, char *list[]){
	int n = 0;

	if(!(mod_list = (struct modules*)malloc(sizeof(struct modules)+sizeof(int)*nmod)))
		return -1;

	for(int i = 0; i < nmod; i++){
		int fd;
		struct stat stbuf;

		if((fd = open(list[i], O_RDONLY)) < 0){
			perror(list[i]);
			continue;
		}

		if(fstat(fd, &stbuf) < -1){
			perror("fstat");
			goto err;
		}

		if(!S_ISREG(stbuf.st_mode)){
			fprintf(stderr, "%s: Not a regular file\n", list[i]);
			goto err;
		}

		mod_list->fds[n++] = fd;
		continue;

err:
		close(fd);
	}

	mod_list->nmod = n;
	if(!(mod_list = (struct modules*)realloc(mod_list, sizeof(struct modules)+sizeof(int)*n)))
		return -1;

	return n;
}

void fini_modules(void){
	if(!mod_list)
		return;

	int nmod = mod_list->nmod;
	for(int i = 0; i < nmod; i++){
		int fd = mod_list->fds[i];
		if(fd >= 0)
			close(fd);
	}

	free(mod_list);
	mod_list = NULL;
}

int load_module(struct vm *vm, int id, uint64_t addr, off_t offset, size_t size){
	int nmod;
	int fd;
	size_t aligned_size;

	if(!mod_list)
		return -1;
	nmod = mod_list->nmod;

	if(id >= nmod)
		return -1;
	fd = mod_list->fds[id];

	if(fd < 0)
		return -1;

	if(!size){
		struct stat stbuf;

		if(fstat(fd, &stbuf) < 0){
			perror("fstat");
			return -1;
		}

		size = stbuf.st_size;
	}

	if((addr = palloc(addr, aligned_size = (size + 0x1000-1) & ~0xfff)) == -1){
		perror("palloc");
		return -1;
	}
	memset(guest2phys(vm, addr), 0, aligned_size);

	if(lseek(fd, offset, SEEK_SET) == -1){
		perror("lseek");
		return -1;
	}

	if(read(fd, guest2phys(vm, addr), size) == -1){
		perror("read");
		return -1;
	}

	return addr;
}

int load_kernel(struct vm *vm){
	return load_module(vm, 0, 0, 0, 0);
}
