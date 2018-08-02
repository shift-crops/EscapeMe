CSRCS := $(filter %.c,$(SRCS))
SSRCS := $(filter %.S,$(SRCS))

OBJS := $(CSRCS:.c=.o) $(SSRCS:.S=.o)
DEPS := $(CSRCS:.c=.d)

ifndef CFLAGS
	CFLAGS := -Wall -fPIE -masm=intel
endif


.PHONY: all
all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

%.o: %.S
	$(CC) -c $^

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
