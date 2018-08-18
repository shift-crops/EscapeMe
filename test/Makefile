SRCS    := $(wildcard *.c)
SHARED_TARGET   := $(SRCS:.c=-shared.elf)
STATIC_TARGET   := $(SRCS:.c=-static.elf)

CFLAGS  := -Wall -fno-stack-protector -fno-PIE
LDFLAGS := -nostdlib

AS		:= nasm

.PHONY: all
#all: $(SHARED_TARGET) $(STATIC_TARGET)
all: $(STATIC_TARGET)

%-shared.elf: %.c start.o ../libc.so
	$(CC) $(CFLAGS) $(LDFLAGS) start.o $< -L.. -lc -o $@
	execstack -c $@

%-static.elf: start.o %.c ../libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	execstack -c $@

%.o: %.s
	$(AS) -f elf64 $^ -o $@

.PHONY: clean
clean:
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)
