
all: soc_system.dtbo

soc_system.dtbo: soc_system.dtsi
	dtc -I dts -O dtb -@ -o $@ $<

clean:
	rm -f *.dtbo *~

.PHONY: dtsi clean
