CSRCS	:= $(wildcard *.c)
SSRCS	:= $(wildcard *.s)
OBJS	:= $(SSRCS:.s=.o) $(CSRCS:.c=.o)
DEPS	:= $(CSRCS:.c=.d)

ifndef CFLAGS
	CFLAGS	:= -Wall -I.. -masm=intel -fPIE -g3
endif

AS		:= nasm

.PHONY: all
all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

%.o: %.s
	$(AS) -f elf64 $^

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
