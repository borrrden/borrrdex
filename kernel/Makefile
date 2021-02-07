
OSNAME = borrrdex

GNUEFI = ../gnu-efi
OVMFDIR = ../OVMFbin
LDS = kernel.ld
CC = gcc
ASMC = nasm
LD = ld

override CFLAGS := -I src -ffreestanding -fshort-wchar -mno-red-zone -g $(CFLAGS)
ASMFLAGS = -g -F dwarf
LDFLAGS = -T $(LDS) -static -Bsymbolic -nostdlib

SRCDIR := src
OBJDIR := lib
BUILDDIR = bin
BOOTEFI := $(GNUEFI)/x86_64/bootloader/main.efi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)
ASMSRC = $(call rwildcard,$(SRCDIR),*.asm)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(ASMSRC))
DIRS = $(wildcard $(SRCDIR)/*)

kernel:	$(OBJS) link

$(OBJDIR)/arch/x86_64/interrupt/%.o: $(SRCDIR)/arch/x86_64/interrupt/%.cpp
	@ echo !== Compiling $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $^ -o $@

$(OBJDIR)/arch/x86_64/irq.o: $(SRCDIR)/arch/x86_64/irq.cpp
	@ echo !== Compiling $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -mgeneral-regs-only -c $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo !== Compiling $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@ echo !== Assembling $^
	@ mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -f elf64 -o $@

link:
	@ echo !== Linking $^
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/kernel.elf $(OBJS)

setup:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(OBJDIR)

buildimg:
	@objcopy --only-keep-debug $(BUILDDIR)/kernel.elf $(BUILDDIR)/kernel.sym
	@strip --strip-debug --strip-unneeded $(BUILDDIR)/kernel.elf
	@objcopy --add-gnu-debuglink=$(BUILDDIR)/kernel.sym $(BUILDDIR)/kernel.elf
	@dd if=/dev/zero of=$(BUILDDIR)/$(OSNAME).img bs=512 count=93750
	@mformat -i $(BUILDDIR)/$(OSNAME).img -f 1440 ::
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI/BOOT
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(BOOTEFI) ::/EFI/BOOT
	@mcopy -i $(BUILDDIR)/$(OSNAME).img startup.nsh ::
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/kernel.elf ::
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/zap-light16.psf ::
	@# mkdir -p $(BUILDDIR)/iso/EFI/BOOT
	@# cp $(BOOTEFI) $(BUILDDIR)/iso/EFI/BOOT
	@# cp startup.nsh $(BUILDDIR)/iso/
	@# cp $(BUILDDIR)/zap-light16.psf $(BUILDDIR)/iso/
	@# cp $(BUILDDIR)/kernel.elf $(BUILDDIR)/iso/
	@# cd $(BUILDDIR)
	@# xorrisofs -r -J -o $(BUILDDIR)/$(OSNAME).iso $(BUILDDIR)/iso

run:
	qemu-system-x86_64 -drive file=$(BUILDDIR)/$(OSNAME).img -m 256M -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none

clean:
	rm -f $(BUILDDIR)/kernel.*
	rm -f $(BUILDDIR)/$(OSNAME).img
	rm -rf $(OBJDIR)