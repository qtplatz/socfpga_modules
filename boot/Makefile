# Makefile
OS := $(shell uname -s)
ARCH := $(shell uname -m)
ifeq (${OS},Windows)
	QUARTUS_DIR := $(firstword $(wildcard C:/*/*/quartus))
else
	QUARTUS_DIR := $(firstword $(wildcard /opt/intelFPGA_lite/20.1/quartus))
endif
export QUARTUS_ROOTDIR=${QUARTUS_DIR}
export PATH := ${PATH}:${QUARTUS_ROOTDIR}/bin:${QUARTUS_ROOTDIR}/sopc_builder/bin

ifeq ($(OS),Linux)
QUARTUS_CPF=${QUARTUS_DIR}/bin/quartus_cpf
else
QUARTUS_CPF=quartus_cpf
endif
QPFDIR=../qpf

device_host ?= "de0"
device_user ?= $(shell whoami)
design_name = soc_system
TARGETS = ${design_name}.dtb ${design_name}.dts ${design_name}.rbf boot.scr

ifeq (${ARCH},armv7l)
DTB=${design_name}.dtb
else
include ../config.sh
KERNEL_DIR = ${HOME}/${cross_kernel_source}
DTB  = ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dtb
ZIMG = ${KERNEL_DIR}/arch/arm/boot/zImage
endif

ifeq (${ARCH},armv7l)
all:
	@echo "do 'sudo make install'"
else
all: ${TARGETS}
endif

test:
	@echo ${KERNEL_DIR}
	@echo ${DTB}

boot.scr: boot.cmd
	mkimage -C none -A arm -T script -d $< $@

ifeq (${ARCH},x86_64)
${design_name}/${design_name}.h: ../qpf/${design_name}.sopcinfo
	if [ ! -d "${design_name}" ]; then mkdir "${design_name}"; fi
	sopc-create-header-files ../qpf/${design_name}.sopcinfo --output-dir "${design_name}"

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h: ${design_name}/${design_name}.h
	cp -p $< $@

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi: ../dtsi/${design_name}_embedded.dtsi
	cp -p $< $@

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts: ../dtsi/${design_name}.dts
	cp -p $< $@

${design_name}.dtb: ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts
	${MAKE} -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ${design_name}.dtb
	cp -p ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dtb $@
endif

${design_name}.dts: ${design_name}.dtb
	dtc -I dtb -O dts -o $@ $<

## ------ rbf --------

${design_name}.rbf: ${QPFDIR}/${design_name}.sof
	${QUARTUS_CPF} -c --option=option_file.resp $< $@

clean:
	rm -rf *.dtb *.dts *.rbf *~ \#* ${design_name}

ifeq (${ARCH},armv7l)
install:
	-mount /dev/mmcblk0p1 /mnt
	cp ${TARGETS} zImage /mnt/
else
install: ${TARGETS}
	scp ${TARGETS} ${ZIMG} Makefile ${device_user}@${device_host}:
endif
