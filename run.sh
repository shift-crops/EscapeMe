#!/bin/sh

# only "-static" elf run
./kvm/kvm.elf kernel/kernel.bin libc/test/memory-static.elf
