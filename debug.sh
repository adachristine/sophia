#!/usr/bin/env bash

make -C kjarna_runtime &&
make -C kjarna &&

BOOT_DIR=EFI/BOOT
BOOT_FILE=BOOTX64.EFI
SYS_DIR=adasoft/sophia
OVMF_DIR=/usr/share/ovmf/x64

LOADER_FILE=kjarna/kjarna.efi
SHIM_FILE=kjarna_runtime/kjarna.os

KERNEL_FILE=kc/core/kernel.os

tempdir=$(mktemp -d) &&
rundir=${tempdir}/run &&

cp ${OVMF_DIR}/OVMF_VARS.fd ${tempdir} &&

mkdir -p ${rundir}/${BOOT_DIR} ${rundir}/${SYS_DIR} &&
cp ${LOADER_FILE} ${rundir}/${BOOT_DIR}/${BOOT_FILE} &&
cp ${LOADER_FILE} ${SHIM_FILE} ${rundir}/${SYS_DIR} &&

qemu-system-x86_64 -d int \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=${tempdir}/OVMF_VARS.fd \
    -net none -drive file=fat:rw:${rundir} -monitor stdio \
    -serial file:sophia.log -no-shutdown -no-reboot -s -S 

rm -rf ${tempdir}

