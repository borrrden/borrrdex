JOBS := $(shell nproc)

.PHONY: all disk fastdisk kernel initrd libc

all: kernel libc base initrd disk

libc:
	ninja -j$(JOBS) -C LibC/build install

system:
	ninja -j$(JOBS) -C System/build install

kernel:
	ninja -j$(JOBS) -C Kernel/build

userspace: system

base: userspace libc

initrd:
	Scripts/buildinitrd.sh

disk:
	Scripts/copytodisk.sh

fastdisk:
	Scripts/copytodisk.sh nomount

clean:
	ninja -C LibC/build clean
	ninja -C Kernel/build clean	
	ninja -C System/build clean
	rm -rf Initrd/*
	rm initrd.tar