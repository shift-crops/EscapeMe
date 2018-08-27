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

    $ ls -al /home/escape
    drwxr-x--- 2 root escape   4096 Aug 25 23:38 .
    drwxrwxr-x 8 root root     4096 Aug 25 23:50 ..
    -rw-r----- 1 root escape     62 Aug 25 21:57 flag2.txt
    -rw-r----- 1 root escape     61 Aug 25 21:57 flag3-sha1_of_flag.txt
    -rw-r----- 1 root escape   8514 Aug 27 12:09 hashcash.pyc
    -rw-r----- 1 root escape   8488 Aug 25 22:31 kernel.bin
    -rwxr-x--- 1 root escape 186720 Aug 25 22:32 kvm.elf
    -rwxr-x--- 1 root escape  20240 Aug 25 22:31 memo-static.elf
    -rwxr-x--- 1 root escape   1583 Aug 27 12:45 pow.py
