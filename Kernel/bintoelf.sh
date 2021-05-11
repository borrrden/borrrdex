#!/bin/sh

# This script exists to properly compile the SMP trampoline assembly file
# because NASM has a lot of trouble properly generating assembly code
# with both 16-bit and 64-bit code.  This two step approach works though

nasm -fbin -i "$3" "$1" -o "$4.bin"
objcopy -I binary -O elf64-x86-64 -B i386 "$4.bin" $2