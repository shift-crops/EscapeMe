#!/bin/sh

make clean -C kvm
make clean -C kernel
make clean -C libc
make clean -C bin
make clean -C exploit
