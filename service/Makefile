TARGET := service.a

include ../template.mk

ifndef CTF_FLAG1
CTF_FLAG1 := XXXXX{1111111111111111111111111111111}
endif

syscall.o: syscall.c
	$(CC) $(CFLAGS) -c -MMD -MP $< -DFLAG1=\"$(CTF_FLAG1)\"
