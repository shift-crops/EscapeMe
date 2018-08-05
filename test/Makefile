SRCS    := $(wildcard ./*.c)
SHARED_TARGET   := $(SRCS:.c=-shared.bin)
STATIC_TARGET   := $(SRCS:.c=-static.bin)

CFLAGS  := -nostdlib -Wall

.PHONY: all
all: $(SHARED_TARGET) $(STATIC_TARGET)

%-shared.bin: %.c ../libc.so
	$(CC) $(CFLAGS) $< -L.. -lc -o $@

%-static.bin: %.c ../libc.a
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)
