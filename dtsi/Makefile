#! Makefile
export QUARTUS_ROOTDIR=/opt/intelFPGA_lite/20.1/quartus
export PATH := ${PATH}:${QUARTUS_ROOTDIR}/bin:${QUARTUS_ROOTDIR}/sopc_builder/bin

all: soc_system.dtbo

soc_system.dtbo: soc_system.dtsi
	dtc -I dts -O dtb -@ -o $@ $<

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
	sudo cp soc_system.dtbo /lib/firmware
	echo "soc_system.dtbo" | sudo tee -a | "/sys/kernel/config/device-tree/overlays/fpga/path"

uninstall:
	if [ -d "/sys/kernel/config/device-tree/overlays/fpga" ]; then\
		sudo rmdir "/sys/kernel/config/device-tree/overlays/fpga";\
	fi

tmp/soc_system.h: ../qpf/soc_system.sopcinfo
	if [ ! -d "sopc" ]; then mkdir "sopc"; fi
	sopc-create-header-files ../qpf/soc_system.sopcinfo --output-dir "tmp"

sopc: tmp/soc_system.h

clean:
	rm -f *.dtbo *~
	rm -rf tmp

.PHONY: clean install uninstall
