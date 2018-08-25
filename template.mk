SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

ifndef CFLAGS
CFLAGS := -Wall -L.. -g3
endif

.PHONY: all
all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP $<

.PHONY: clean
clean:
	$(RM) $(DEPS) $(OBJS) $(TARGET)
