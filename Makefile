JOBS := $(shell nproc)

.PHONY: all disk fastdisk kernel initrd libc

all: kernel libc initrd disk

libc:
	ninja -j$(JOBS) -C LibC/build install

kernel:
	ninja -j$(JOBS) -C Kernel/build

initrd:
	Scripts/buildinitrd.sh

disk:
	Scripts/copytodisk.sh

fastdisk:
	Scripts/copytodisk.sh nomount

clean:
	ninja -C LibC/build clean
	ninja -C Kernel/build clean