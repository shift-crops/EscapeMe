SRCS    := $(wildcard ./*.c)
SHARED_TARGET   := $(SRCS:.c=-shared.elf)
STATIC_TARGET   := $(SRCS:.c=-static.elf)

CFLAGS  := -nostdlib -Wall

.PHONY: all
all: $(SHARED_TARGET) $(STATIC_TARGET)

%-shared.elf: %.c ../libc.so
	$(CC) $(CFLAGS) $< -L.. -lc -o $@

%-static.elf: %.c ../libc.a
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)
