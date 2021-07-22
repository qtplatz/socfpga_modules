
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

clean:
	rm -f *.dtbo *~

.PHONY: dtsi clean
