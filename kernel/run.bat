set OSNAME=borrrdex
set BUILDDIR=%0/../bin
set OVMFDIR=%0/../../OVMFbin
set BUILDDIR=%BUILDDIR:"=%
set OVMFDIR=%OVMFDIR:"=%

set PATH=C:\Program Files\qemu;%PATH%
qemu-system-x86_64 -drive file=%BUILDDIR%/%OSNAME%.img -serial vc -m 256M -usb -cpu qemu64 -machine q35 -drive if=pflash,format=raw,unit=0,file=%OVMFDIR%/OVMF_CODE-pure-efi.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=%OVMFDIR%/OVMF_VARS-pure-efi.fd -net none -s
::pause
