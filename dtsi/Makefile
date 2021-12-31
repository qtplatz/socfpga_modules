#! Makefile

include ../config.sh
ARCH := $(shell uname -m)
export QUARTUS_ROOTDIR=/opt/intelFPGA_lite/20.1/quartus
export PATH := ${PATH}:${QUARTUS_ROOTDIR}/bin:${QUARTUS_ROOTDIR}/sopc_builder/bin

device_host ?= "de0"
device_user ?= $(shell whoami)
design_name = soc_system
KERNEL_DIR = ${HOME}/${cross_kernel_source}

DTS_SOURCES = ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_overlay.dts \
		${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi \
		${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts

all:
	@echo "	make header		-- generate ${design_name}/${design_name}.h"
	@echo "	make dtb		-- generate ${design_name}.dtb"

#       @echo "	make copy_dts		-- generate ${design_name}_overlay.dts in ${KERNEL_DIR}/arch/arm/boot/dts"
#	@echo "	make dtbo		-- generate ${design_name}.dtbo"

header: ${design_name}/${design_name}.h
copy_dts: ${DTS_SOURCES} # ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h
dtbo: ${design_name}.dtbo ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_overlay.dts
dtb: ${design_name}.dtb \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts

#soc_system.dtbo: soc_system.dtsi
#	dtc -I dts -O dtb -@ -o $@ $<

ifeq (${ARCH},armv7l)
status:
	@if [ -d "/sys/kernel/config/device-tree/overlays/fpga" ];\
		then @cat /sys/kernel/config/device-tree/overlays/fpga/status; \
	else \
		echo "not installed."; \
	fi

install:
	@if [ ! -d "/lib/firmware" ]; then sudo mkdir -p /lib/firmware; fi
	@if [ ! -d "/sys/kernel/config/device-tree/overlays/fpga" ]; then \
		sudo mkdir "/sys/kernel/config/device-tree/overlays/fpga";\
	fi
	sudo cp ${design_name}.dtbo /lib/firmware
	echo "${design_name}.dtbo" | sudo tee -a | "/sys/kernel/config/device-tree/overlays/fpga/path"

uninstall:
	if [ -d "/sys/kernel/config/device-tree/overlays/fpga" ]; then\
		sudo rmdir "/sys/kernel/config/device-tree/overlays/fpga";\
	fi
else
install:
	scp ${design_name}.dtbo ${device_user}@${device_host}:
endif

ifeq (${ARCH},x86_64)
${design_name}/${design_name}.h: ../qpf/${design_name}.sopcinfo
	if [ ! -d "${design_name}" ]; then mkdir "${design_name}"; fi
	sopc-create-header-files ../qpf/${design_name}.sopcinfo --output-dir "${design_name}"

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h: ${design_name}/${design_name}.h
	cp -p $< $@

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_overlay.dts: ${design_name}_overlay.dts
	cp -p $< $@

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts: ${design_name}.dts
	cp -p $< $@

${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi: ${design_name}_embedded.dtsi
	cp -p $< $@

${design_name}.dtbo: ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_overlay.dts
	make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ${design_name}_overlay.dtb
	cp -p ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_overlay.dtb $@

${design_name}.dtb: ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.h \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}_embedded.dtsi \
	${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dts
	make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ${design_name}.dtb
	cp -p ${KERNEL_DIR}/arch/arm/boot/dts/${design_name}.dtb $@
endif

clean:
	rm -f *.dtbo *~
	rm -rf ${design_name}

.PHONY: clean install uninstall
