OUTDIR      := release

SUB_OBJS    := kvm/kvm.elf kernel/kernel.bin bin/memo-static.elf
TARGET      := $(addprefix $(OUTDIR)/,$(notdir $(SUB_OBJS)))
FLAG_FILE   := flag2.txt flag3-*.txt
FLAG_TARGET := $(addprefix $(OUTDIR)/,$(FLAG_FILE))
EXPLOIT     := exploit/exploit.elf

CTF_FLAG1   := XXXXX{11111111111111111111111111111111}
CTF_FLAG2   := XXXXX{22222222222222222222222222222222}
CTF_FLAG3   := XXXXX{33333333333333333333333333333333}

export CTF_FLAG1

.PHONY: all
all: $(TARGET) $(FLAG_TARGET)

$(TARGET): $(SUB_OBJS)
	cp $^ release
	strip release/kvm.elf

$(OUTDIR)/flag2.txt:
	echo "Here is second flag : ${CTF_FLAG2}" > $(OUTDIR)/flag2.txt

$(OUTDIR)/flag3-*.txt:
	echo "Here is third flag : ${CTF_FLAG3}" > $(OUTDIR)/flag3-`echo ${CTF_FLAG3} | sha1sum | cut -d' ' -f1`.txt

$(EXPLOIT): kernel/kernel.elf
	$(MAKE) -C exploit SYS_HANDLER=0x`nm $< | grep syscall_handler | cut -d' ' -f1`

$(SUB_OBJS) kernel/kernel.elf: FORCE
	$(MAKE) -C $(dir $@) $(notdir $@)


.PHONY: run
run: $(TARGET) $(FLAG_TARGET)
	$(OUTDIR)/run.sh

.PHONY: exploit
exploit: $(TARGET) $(FLAG_TARGET) $(EXPLOIT)
	cd exploit && ./exploit.py

.PHONY: clean
clean:
	dirname $(SUB_OBJS) | xargs -l $(MAKE) clean -C
	$(RM) $(TARGET) $(FLAG_TARGET) $(EXPLOIT)

FORCE:
