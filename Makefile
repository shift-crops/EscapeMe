OUTDIR      := release

SUB_OBJS    := kvm/kvm.elf kernel/kernel.bin bin/memo-static.elf
TARGET      := $(addprefix $(OUTDIR)/,$(notdir $(SUB_OBJS)))
FLAG_FILE   := flag2.txt flag3-*.txt
FLAG_TARGET := $(addprefix $(OUTDIR)/,$(FLAG_FILE))
EXPLOIT     := exploit/exploit.elf

ifdef FLAG
CTF_FLAG1   := ${FLAG}{fr33ly_3x3cu73_4ny_5y573m_c4ll}
CTF_FLAG2   := ${FLAG}{ABI_1nc0n51573ncy_l34d5_70_5y573m_d357ruc710n}
CTF_FLAG3   := ${FLAG}{Or1g1n4l_Hyp3rc4ll_15_4_h07b3d_0f_bug5}
FLAG3_NAME  := flag3-`echo -n ${CTF_FLAG3} | sha1sum | cut -d' ' -f1`.txt
else
CTF_FLAG1   := XXXXX{11111111111111111111111111111111}
CTF_FLAG2   := XXXXX{22222222222222222222222222222222}
CTF_FLAG3   := XXXXX{33333333333333333333333333333333}
FLAG3_NAME  := flag3-sha1_of_flag.txt
endif

export CTF_FLAG1

.PHONY: all
all: $(TARGET) $(FLAG_TARGET)

$(TARGET): $(SUB_OBJS)
	cp $^ release
	strip -K main -K palloc release/kvm.elf
	strip --strip-debug release/memo-static.elf

$(OUTDIR)/flag2.txt:
	echo "Here is second flag : ${CTF_FLAG2}" > $(OUTDIR)/flag2.txt

$(OUTDIR)/flag3-*.txt:
	echo "Here is final flag : ${CTF_FLAG3}" > $(OUTDIR)/$(FLAG3_NAME)

$(EXPLOIT): kernel/kernel.elf
	$(MAKE) -C exploit SYS_HANDLER=0x0`nm $< | grep syscall_handler | cut -d' ' -f1`

$(SUB_OBJS) kernel/kernel.elf: FORCE
	$(MAKE) -C $(dir $@) $(notdir $@)


.PHONY: run
run: $(TARGET) $(FLAG_TARGET)
	$(OUTDIR)/run.sh

.PHONY: exploit
exploit: $(TARGET) $(FLAG_TARGET) $(EXPLOIT)
	cd exploit && ./exploit.py

.PHONY: release
release: $(TARGET) $(FLAG_TARGET)
	bash -c "cd $(OUTDIR) && tar zcf EscapeMe.tar.gz {*.elf,*.bin,*.so,*.txt,pow.py}"

.PHONY: clean
clean:
	dirname $(SUB_OBJS) | xargs -l $(MAKE) clean -C
	$(MAKE) clean -C exploit
	$(RM) $(TARGET) $(FLAG_TARGET) $(OUTDIR)/EscapeMe.tar.gz

FORCE:
