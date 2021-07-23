# Makefile
OS := $(shell uname -s)
ARCH := $(shell uname -m)

ifeq (${OS},Windows)
	QUARTUS_DIR := $(firstword $(wildcard C:/*/*/quartus))
else
	QUARTUS_DIR := $(firstword $(wildcard /opt/intelFPGA_lite/20.1/quartus))
endif

ifeq ($(OS),Linux)
QUARTUS_CPF=${QUARTUS_DIR}/bin/quartus_cpf
else
QUARTUS_CPF=quartus_cpf
endif
QPFDIR=../qpf

device_host ?= "de0"
device_user ?= $(shell whoami)
design_name = soc_system
TARGETS  = ${design_name}.dtb ${design_name}.dts ${design_name}.rbf boot.scr

ifeq (${ARCH},armv7l)
DTB=${design_name}.dtb
else
KERNEL_DIR=../../linux-socfpga
#DTB  = ${KERNEL_DIR}/arch/arm/boot/dts/socfpga_cyclone5_de0_nano_soc.dtb # <-- original
DTB  = ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dtb
ZIMG = ${KERNEL_DIR}/arch/arm/boot/zImage
endif

ifeq (${ARCH},armv7l)
all:
	@echo "do 'sudo make install'"
else
all: ${TARGETS}
endif

boot.scr: boot.cmd
	mkimage -C none -A arm -T script -d $< $@

${DTB}: ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts
	make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ${design_name}.dtb
	cp -p ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dtb $@

${design_name}.dtb: ${DTB}
	cp -p ${DTB} $@

${design_name}.dts: ${DTB}
	dtc -I dtb -O dts -o $@ $<

${design_name}.rbf: ${QPFDIR}/${design_name}.sof
	${QUARTUS_CPF} -c --option=option_file.resp $< $@

clean:
	rm -rf *.dtb *.dts *.rbf *~ \#*

ifeq (${ARCH},armv7l)
install:
	-mount /dev/mmcblk0p1 /mnt
	cp ${TARGETS} zImage /mnt/
else
install: ${TARGETS}
	scp ${TARGETS} ${ZIMG} Makefile ${device_user}@${device_host}:
endif
