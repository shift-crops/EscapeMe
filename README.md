# EscapeMe

## Environment

Ubuntu 18.04

## Requirement

- make
- gcc
- nasm
- execstack

## Usage

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

## Attachement

- pow.py
- kvm.elf
- kernel.bin (including flag1)
- memo-static.elf
- libc-2.27.so
- flag2.txt
- flag3-sha1\_of\_flag.txt

## Deployment on CTF Server

    $ ls -al /home/escape/
    drwxr-x--- 2 root escape  4096 Aug 28 11:28 .
    drwxr-xr-x 8 root root    4096 Aug 28 10:10 ..
    -rw-r----- 1 root escape    75 Aug 28 10:10 flag2.txt
    -rw-r----- 1 root escape    67 Aug 28 10:10 flag3-415254a0b8be92e0a976f329ad3331aa6bbea816.txt
    -rw-r----- 1 root escape  8514 Aug 28 10:10 hashcash.pyc
    -rw-r----- 1 root escape  8544 Aug 28 10:10 kernel.bin
    -rwxr-x--- 1 root escape 23336 Aug 28 10:10 kvm.elf
    -rwxr-x--- 1 root escape 20176 Aug 28 10:10 memo-static.elf
    -rwxr-x--- 1 root escape  1684 Aug 28 11:28 pow.py
