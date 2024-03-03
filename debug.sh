#!/usr/bin/env bash

BOOT_DIR=boot
BOOT_FILE=${BOOT_DIR}/kjarna.efi

SERVICE_DIR=service
SERVICE_FILE=${SERVICE_DIR}/kjarna.os

FIRMWARE_DIR=/usr/share/ovmf/x64
FIRMWARE_FILE=${FIRMWARE_DIR}/OVMF_CODE.fd
FIRMWARE_VARS=${FIRMWARE_DIR}/OVMF_VARS.fd

INSTALL_PREFIX=adasoft/sophia

make -C ${SERVICE_DIR} &&
make -C ${BOOT_DIR} &&

tempdir=$(mktemp -d) &&
rundir=${tempdir}/run

varsfile=${tempdir}/$(basename ${FIRMWARE_VARS})

efidir=${rundir}/efi
efibootdir=${efidir}/boot
bootdir=${efidir}/adasoft/kjarna
bootfile=${efibootdir}/BOOTX64.EFI
servdir=${rundir}/service/kjarna

mkdir -p ${efidir} ${efibootdir} &&
mkdir -p ${bootdir} ${servdir} &&

cp ${FIRMWARE_VARS} ${varsfile} &&
cp ${BOOT_FILE} ${bootdir} &&
cp ${BOOT_FILE} ${bootfile} &&
cp ${SERVICE_FILE} ${servdir} &&

qemu-system-x86_64 -d int \
    -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/x64/OVMF_CODE.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=${varsfile} \
    -net none -drive file=fat:rw:${rundir} -monitor stdio \
    -serial file:sophia.log -no-shutdown -no-reboot -s -S 

rm -rf ${tempdir}

