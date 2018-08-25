CSRCS  := $(wildcard *.c)
SSRCS  := $(wildcard *.s)

OBJS   := $(CSRCS:.c=.o) $(SSRCS:.s=.o)
DEPS   := $(CSRCS:.c=.d)

ifndef CFLAGS
CFLAGS := -Wall -masm=intel -fno-stack-protector -fPIE
endif

.PHONY: all
all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

%.o: %.s
	$(CC) -c $^

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
