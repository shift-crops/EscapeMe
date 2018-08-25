#!/usr/bin/env bash

cd `dirname $0`

make -C kvm
make -C kernel
make -C bin
make -C exploit

cp -n kvm/kvm.elf kernel/kernel.bin bin/memo-static.elf release
strip release/kvm.elf
