set PATH=C:\Program Files\qemu;%PATH%
start qemu-system-x86_64 Z:\home\borrrden\borrrdex2\Disks\Borrrdex.img^
    -no-reboot -no-shutdown^
    -m 1024m^
    -device qemu-xhci^
    -machine q35 ^
    -s -S^
    -serial stdio^
    -rtc base=localtime,clock=host^
    -device intel-hda -device hda-duplex^
    -smp cores=2,sockets=1
::pause
