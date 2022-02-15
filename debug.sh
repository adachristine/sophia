#!/usr/bin/env bash

make -C kc/core &&
make -C kc/boot &&
make -C loader &&

./image.sh &&

qemu-system-x86_64 -d int \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
    -net none -drive file=uefi.img,if=ide,format=raw -monitor stdio \
    -serial file:sophia.log -no-shutdown -no-reboot -s -S

