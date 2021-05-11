JOBS := $(shell nproc)

.PHONY: all disk kernel initrd libc

all: kernel libc initrd disk

libc:
	ninja -j$(JOBS) -C LibC/build install

kernel:
	ninja -j$(JOBS) -C Kernel/build

initrd:
	Scripts/buildinitrd.sh

disk:
	Scripts/copytodisk.sh

clean:
	ninja -C LibC/build clean
	ninja -C Kernel/build clean