#!/usr/bin/env bash

make -C loader &&
make -C kernel &&

./image.sh &&

qemu-system-x86_64 -cpu qemu64 \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
    -net none -drive file=uefi.img,if=ide -monitor stdio -serial file:sophia.log 

