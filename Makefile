JOBS := $(shell nproc)

.PHONY: all disk kernel base initrd libc

all: kernel libc base initrd disk

libc:
	ninja -j$(JOBS) -C LibC/build install

kernel:
	ninja -j$(JOBS) -C Kernel/build

clean:
	ninja -C LibC/build clean
	ninja -C Kernel/build clean