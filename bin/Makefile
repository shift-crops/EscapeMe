AS		:= nasm

SRCS            := $(wildcard *.c)
SHARED_TARGET   := $(SRCS:.c=-shared.elf)
STATIC_TARGET   := $(SRCS:.c=-static.elf)

CFLAGS  := -Wall -fno-stack-protector -fno-PIE
LDFLAGS := -nostdlib -no-pie

.PHONY: all
all: $(SHARED_TARGET) $(STATIC_TARGET)

%-shared.elf: %.c start.o libc/libc.so
	$(CC) $(CFLAGS) $(LDFLAGS) start.o $< -L./libc -lc -o $@
	execstack -c $@

%-static.elf: start.o %.c libc/libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	execstack -c $@

libc/libc.so libc/libc.a:
	$(MAKE) -C libc

%.o: %.s
	$(AS) -f elf64 $^

.PHONY: clean
clean:
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)
	$(MAKE) clean -C libc
