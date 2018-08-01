TARGET := libc.so

SUB_OBJS := stdlib/stdlib.a malloc/malloc.a assert/assert.a misc/misc.a
EXPORT   := export.map

CFLAGS   := -Wall -fPIE -g3
LDFLAGS  := -shared -pie -nostdlib -E --version-script=$(EXPORT)

.PHONY: all
all: $(TARGET)

$(TARGET): $(SUB_OBJS)
	$(LD) $(LDFLAGS) --whole-archive $^ -o $@

$(SUB_OBJS): FORCE
	make -C `dirname $@` CFLAGS="$(CFLAGS)" `basename $@`
	#make -C `dirname $@` `basename $@`

.PHONY: clean
clean: 
	dirname $(SUB_OBJS) | xargs -l make clean -C
	$(RM) $(TARGET)

FORCE:
