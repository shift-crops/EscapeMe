# EscapeMe

## environment

Ubuntu 18.04

## requirement

- make
- gcc
- nasm
- execstack

## usage

Build

    $ make
    $ make FLAG=TWCTF  # generate real flag

Run

    $ make run

    $ make
    $ ./release/run.sh

Exploit

    $ make exploit

    $ make
    $ cd exploit && ./exploit.py

Clean

    $ make clean

## attachement

- run.sh
- kvm.elf
- kernel.bin (including flag1)
- memo-static.elf
- libc-2.27.so
- flag2.txt
- flag3-sha1\_of\_flag.txt
