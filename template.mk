OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

#CFLAGS := -Wall -L.. -g3

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
