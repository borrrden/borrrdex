set OSNAME=borrrdex
set BUILDDIR=%0/../bin
set OVMFDIR=%0/../../OVMFbin
set BUILDDIR=%BUILDDIR:"=%
set OVMFDIR=%OVMFDIR:"=%

set PATH=C:\Program Files\qemu;%PATH%
qemu-system-x86_64 -drive file=%BUILDDIR%/%OSNAME%.img^
    -m 256M^
    -cpu qemu64^
    -drive if=pflash,format=raw,unit=0,file=%OVMFDIR%/OVMF_CODE-pure-efi.fd,readonly=on^
    -drive if=pflash,format=raw,unit=1,file=%OVMFDIR%/OVMF_VARS-pure-efi.fd^
    -machine q35 ^
    -s
    ::-rtc base=localtime,clock=host^
    ::-usb^
    ::-serial file:%BUILDDIR%/serial.log^
    ::-netdev user,id=en0 -nic user,model=e1000^
::pause
