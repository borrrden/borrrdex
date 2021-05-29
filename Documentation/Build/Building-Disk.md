With the sysroot completed, the next step is to create a disk image that can be run with an emulator like Qemu, or installed to a USB drive or even a hard disk.  The end result of this is a formatted disk with three partitions on it, each formatted with the appropriate filesystem (if any).

### Disk creation
------------------

[`dd`](https://www.man7.org/linux/man-pages/man1/dd.1.html) is utilized to physically create the bytes for the disk image.  It uses `/dev/zero` in order to create a file 1GB in size full of nothing but zeros.  However, this is useful because it simulates a blank slate disk that partitioning tools can work with.

### Disk partitioning
-------------------

Once the blank disk image exists, the next step is to partition it and write its partition table.  The GPT partition scheme is used in this case, rather than the legacy MBR partitioning scheme.  This is because nearly every machine made in the past decade supports it and GPT is backwards compatible with MBR anyway.  The program used to partition the disk is called [`sfdisk`](https://www.man7.org/linux/man-pages/man8/sfdisk.8.html).  You can see the script that is sent to it for partition by looking at [partitions.sfdisk](../../Scripts/partitions.sfdisk).  Here is a quick breakdown of what that script creates:

- A 1MiB BIOS boot drive starting at sector 2048.  The reason for starting at 2048 is not relevant here, but it is a round number that is a multiple of various hardware sector sizes and thus is the default starting sector for (probably) every modern partitioning program.  This will store the fallback bootloader in the case that EFI cannot be used.
- A 128 MiB EFI partition at the end of the drive for storing the UEFI boot code that is the primary method of booting the machine.
- 893 MiB in between as a Linux filesystem partition (this will be the system partition) which stores the actual operating system.

Now the disk image will have three partitions on it, but they are still not formatted.

### Disk Formatting
------------------

Since we now have a properly partitioned disk image, it can be mounted like any other disk image using the [`mount`](https://www.man7.org/linux/man-pages/man8/mount.8.html) command.  This step is part of the reason why WSL 1 will not work for building this OS.  It requires the use of a loopback device, which is just a fancy way of saying a device that points to a file that acts as a block storage device.  After setting it up with the image from the previous step, `mount` can be used to mount the various partitions on it.  

Two of the three partitions on the disk will be formatted.  The unformatted one, the BIOS boot partition, doesn't contain any files.  It is just purely flat machine code written into the partition that is loaded into memory at boot time when needed.

The EFI partition is formatted as FAT32 because this is the only filesystem that is guaranteed to be supported by any given UEFI firmware implementation.  This is done using the [`mkfs.fat`](https://www.man7.org/linux/man-pages/man8/mkfs.fat.8.html) command.  It uses all default sizes that FAT32 defines.

The system partition is formatted as ext2 using the [`mkfs.ext2`](https://linux.die.net/man/8/mkfs.ext2) command.  The only thing it specifies is that the filesystem will use a 4096 byte block size (the block size on ext2 is variable, but 4096 is a very common choice over the default 1024 and allows more information to be placed together in the same area).

### Final Step
---------

Once the disk is properly formatted, the EFI bootloader files that limine created are copied to the EFI partition in the proper locations, and `limine-install` is used to install the fallback MBR bootloader code. AFter this the disk image is ready to be used in future steps, and these steps do not need to be repeated. 