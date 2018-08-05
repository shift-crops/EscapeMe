TARGET	:= kernel.elf kernel.bin
#TARGET	:= kernel.bin

SRCS 	:= kernel.c startup.s hypercall.s misc.s
CSRCS	:= $(filter %.c,$(SRCS))
SSRCS	:= $(filter %.s,$(SRCS))

OBJS	:= $(SSRCS:.s=.o) $(CSRCS:.c=.o)
DEPS	:= $(CSRCS:.c=.d)

AS		:= nasm

CFLAGS	:= -Wall -fno-stack-protector -g3 -masm=intel
LDFLAGS	:= -nostdlib -Ttext 0x0

.PHONY: all
all: $(TARGET)

-include $(DEPS)

%.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.bin: $(OBJS)
	$(LD) $(LDFLAGS) --oformat=binary $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

%.o: %.s
	$(AS) -f elf64 $^

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
