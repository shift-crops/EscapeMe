TARGET  := kvm.elf
SRCS	:= main.c

OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
SUB_OBJS := vm/vm.a utils/utils.a
CFLAGS   := -Wall -Wl,-z,relro,-z,now -fstack-protector -I.. -g3

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS) $(SUB_OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

$(SUB_OBJS): FORCE
	$(MAKE) -C $(dir $@) CFLAGS="$(CFLAGS)" $(notdir $@)

.PHONY: clean
clean:
	dirname $(SUB_OBJS) | xargs -l $(MAKE) clean -C
	$(RM) $(DEPS) $(OBJS) $(TARGET)

FORCE:
