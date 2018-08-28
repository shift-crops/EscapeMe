#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int data = 0xdeadbeef;
int bss;

int main(void){
	int stack;
	void *heap, *mmaped[2];

	heap = malloc(0x10);
	mmaped[0] = mmap((void*)0x80000000, 0x1000, 3, 0x22, -1, 0);
	mmaped[1] = mmap(NULL, 0x1000, 3, 0x22, -1, 0);

	printf(	"text 		: %p\n"
			"data 		: %p\n"
			"bss 		: %p\n"
			"heap 		: %p\n"
			"mmaped 1 	: %p\n"
			"mmaped 2 	: %p\n"
			"stack		: %p\n"
			, main, &data, &bss, heap, mmaped[0], mmaped[1], &stack);

	for(int i=0; i<2; i++)
		munmap(mmaped[i], 0x1000);
	free(heap);
}
