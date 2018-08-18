#!/bin/sh

cd `dirname $0`

# only "-static" elf run
./kvm.elf kernel.bin memo-static.elf flag2.txt
