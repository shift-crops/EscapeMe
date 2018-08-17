#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define BUF_SIZE	128
#define MEMO_SIZE	0x28
#define MEMOS		0x10

struct memo {
	char *data;
	int edited;
} *memo;

int menu(void);
void alloc(void);
void edit(void);
void delete(void);
int select_id(void);

int getnline(char *buf, int len);
int getint(void);

int main(void){
	puts("==== secret memo service ====");

	memo = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	for(;;){
		switch(menu()){
			case 1:
				alloc();
				break;
			case 2:
				edit();
				break;
			case 3:
				delete();
				break;
			case 0:
				goto end;
			default:
				puts("Wrong input.");
		}
	}

end:
	munmap(memo, 0x1000);
	puts("Bye!");

	return 0;
}

int menu(void){
	printf(	"\nMENU\n"
			"1. Alloc\n"
			"2. Edit\n"
			"3. Delete\n"
			"0. Exit\n"
			"> ");

	return getint();
}

void alloc(void){
	int id, n;

	for(id = 0; id < MEMOS; id++)
		if(!memo[id].data)
			break;

	if(id >= MEMOS){
		puts("Entry is FULL...");
		return;
	}

	memo[id].data = (char*)calloc(MEMO_SIZE, 1);
	memo[id].edited = 0;

	printf("Input memo > ");
	n = read(0, memo[id].data, MEMO_SIZE);

	printf("Added id:%d entry (%d bytes)\n", id, n);
}

void edit(void){
	int id, n;

	if((id = select_id()) < 0)
		return;

	if(memo[id].edited){
		puts("You already edited this entry...");
		return;
	}

	printf("Input memo > ");
	n = read(0, memo[id].data, strlen(memo[id].data));
	memo[id].edited = 1;

	printf("Edited id:%d entry (%d bytes)\n", id, n);
}

void delete(void){
	int id;
	if((id = select_id()) < 0)
		return;

	free(memo[id].data);
	memo[id].data = NULL;

	printf("Deleted id:%d entry\n", id);
}

int select_id(void){
	int id;

	printf("Input id > ");
	id = getint();

	if(id < 0 || id > MEMOS){
		puts("Invalid id...");
		return -1;
	}

	if(!memo[id].data){
		puts("Entry does not exist...");
		return -1;
	}

	return id;
}

int getnline(char *buf, int size){
	int len;
	char *lf;

	if(size < 0)
		return 0;

	len = read(0, buf, size-1);
	buf[len] = '\0';

	if((lf=strchr(buf,'\n')))
		*lf='\0';

	return len;
}

int getint(void){
	char buf[BUF_SIZE];

	getnline(buf, sizeof(buf));
	return atoi(buf);
}
