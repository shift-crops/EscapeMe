#!/usr/bin/env bash

cd `dirname $0`
set -f

stdbuf -i0 -o0 -e0 echo -n "Any other modules? > "
read mod

if [[ $mod == */* ]]; then
	echo "You can load modules only in this directory"
else
	# only "-static" elf run
	exec ./kvm.elf kernel.bin memo-static.elf $mod
fi
