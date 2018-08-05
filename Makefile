SHARED_TARGET  := libc.so
STATIC_TARGET  := libc.a

SUB_OBJS := stdlib/stdlib.a io/io.a malloc/malloc.a string/string.a \
			libio/libio.a assert/assert.a misc/misc.a
EXPORT   := export.map

CFLAGS   := -Wall -fPIE -g3 -masm=intel
LDFLAGS  := -shared -pie -nostdlib -E --version-script=$(EXPORT)

.PHONY: all
all: $(SHARED_TARGET) $(STATIC_TARGET)
	$(MAKE) -C test

$(SHARED_TARGET): $(SUB_OBJS)
	$(LD) $(LDFLAGS) --whole-archive $^ -o $@

$(STATIC_TARGET): $(SUB_OBJS)
	$(AR) cqT _$@ $^
	echo "create $@\naddlib _$@\nsave\nend" | ar -M
	$(RM) _$@

$(SUB_OBJS): FORCE
	$(MAKE) -C $(dir $@) CFLAGS="$(CFLAGS)" $(notdir $@)

.PHONY: clean
clean: 
	dirname $(SUB_OBJS) | xargs -l $(MAKE) clean -C
	$(MAKE) clean -C test
	$(RM) $(SHARED_TARGET) $(STATIC_TARGET)

FORCE:
