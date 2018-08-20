#!/bin/sh

cd `dirname $0`

read -p "Any other modules? > " mod

# only "-static" elf run
exec ./kvm.elf kernel.bin memo-static.elf $mod
