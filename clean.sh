#!/usr/bin/env bash

cd `dirname $0`

make clean -C kvm
make clean -C kernel
make clean -C bin
make clean -C exploit

rm release/*.{elf,bin}
