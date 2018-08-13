#!/bin/sh

make -C kvm
make -C kernel
make -C libc
make -C bin
