TARGET := kvm.elf
SRCS := vm.c kvm_handler.c gmalloc.c debug.c

OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

CFLAGS := -Wall -g3

.PHONY: all
all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
