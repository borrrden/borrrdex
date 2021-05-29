## Prerequisites
* A UNIX-like build system (WSL 2 will work, WSL 1 will not)
* An existing GCC toolchain
* [Meson](https://mesonbuild.com/Getting-meson.html)
* [CMake](https://cmake.org)

As well as the following (Arch) packages:\
`base-devel`, `autoconf`, `python3`, `ninja`, `wget`, `qemu`, `pip`, `nasm`\
Or Debian, etc.:\
`build-essential`, `autoconf`, `libtool`, `python3`, `python3-pip`, `ninja-build`, `qemu-utils`, `nasm`\
For building the disk image:\
`e2fsprogs`, `dosfstools`

## Cloning
Make sure you use `--recursive` to get the submodules
`git clone https://github.com/borrrden/borrrdex --recursive`

If you have an older version of git that doesn't have this flag you can do it in two steps:
```
git clone https://github.com/borrrden/borrrdex
cd borrrdex/
git submodule update --init --recursive
```

## Building
The build process is divided into parts.  You must follow these parts in order.  Do not just run `make` even though a Makefile is present in the root.  The Makefile will not function unless everything is prepared first.

### Part 1: Building the toolchain

`Toolchain/buildtoolchain.sh build`

This step will build the tools used to build all the parts of the operating system.  The end result is a compiler with an OS-specific target (based on Clang 11.x), a binutils that is aware of the OS, and a limine installation.

[Details Here](Building-Toolchain.md)

### Part 2: Preparing the Build Targets

`Scripts/configure.sh`

This step will use CMake and Meson, two programs that generate build projects, to set up some various things that the operating system needs, as well as some userspace libraries.  Among the things setup or built in this step are the following:

- libc (setup and built)
- kernel (setup)
- Userspace OS library LibBor (setup)
- Userspace third party library libfreetype (setup and built)
- Userspace third party library libressl (setup and built)

[Details Here](Building-Setup.md)

### Part 3: Creating the Disk Image

`Scripts/createdisk.sh`

This step will generate a 1GB disk image that is suitable for running the operating system from.  It will include three partitions:  FAT32 EFI partition, main ext2 partition, and a BIOS boot partition containing legacy MBR things.

[Details Here](Building-Disk.md)

### Part 4: Ready to Build

`make XXX`

Now things are ready to use the top level makefile.  It has quite a few targets, but running `make all` will build all of them.  At the moment this builds the kernel, mlibc again, LibBor, and a userspace console program that will run as the initial process.