#!/bin/sh

# only "-static" elf run
./kvm/kvm.elf kernel/kernel.bin bin/memo-static.elf
