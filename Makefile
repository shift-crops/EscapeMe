TARGET  := kvm.elf

SUB_OBJS := vm/vm.a utils/utils.a
CFLAGS   := -Wall -I.. -g3

.PHONY: all
all: $(TARGET)

$(TARGET): $(SUB_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(SUB_OBJS): FORCE
	$(MAKE) -C $(dir $@) CFLAGS="$(CFLAGS)" $(notdir $@)

.PHONY: clean
clean:
	dirname $(SUB_OBJS) | xargs -l $(MAKE) clean -C
	$(MAKE) clean -C test
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)

FORCE:
