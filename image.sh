#!/usr/bin/env bash

imagefile=uefi.img
loaderfile=loader/loader.efi
kernelfile=kernel/kernel.os

if [[ ! -f ${imagefile} ]]; then
    dd if=/dev/zero of=${imagefile} bs=1024 count=1440
    mformat -i uefi.img -f1440 ::
fi

mmd -i ${imagefile} -Do ::EFI
mmd -i ${imagefile} -Do ::EFI/BOOT
mcopy -i ${imagefile} -Do ${loaderfile} ::EFI/BOOT/BOOTX64.EFI
mmd -i ${imagefile} -Do ::adasoft
mmd -i ${imagefile} -Do ::adasoft/sophia
mcopy -i ${imagefile} ${kernelfile} -Do ::adasoft/sophia

