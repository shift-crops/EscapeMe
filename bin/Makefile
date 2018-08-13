SRCS    := $(wildcard ./*.c)
SHARED_TARGET   := $(SRCS:.c=-shared.elf)
STATIC_TARGET   := $(SRCS:.c=-static.elf)

CFLAGS	:= -nostdlib -Wall

AS		:= nasm

.PHONY: all
all: $(SHARED_TARGET) $(STATIC_TARGET)

%-shared.elf: %.c start.o ../libc/libc.so
	$(CC) $(CFLAGS) start.o $< -L.. -lc -o $@

%-static.elf: start.o %.c ../libc/libc.a
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.s
	$(AS) -f elf64 $^

.PHONY: clean
clean:
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)
