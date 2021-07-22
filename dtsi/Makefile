
all: soc_system.dtbo

soc_system.dtbo: soc_system.dtsi
	dtc -I dts -O dtb -@ -o $@ $<

install:
	sudo mkdir -p /lib/firmware
	sudo mkdir /lib/firmware/soc_system.dtbo /sys/kernel/config/device-tree/overlays/fpga
	sudo cp soc_system.dtbo /lib/firmware
	sudo cp /lib/firmware/soc_system.dtbo /sys/kernel/config/device-tree/overlays/fpga/dtbo

clean:
	rm -f *.dtbo *~

.PHONY: dtsi clean
