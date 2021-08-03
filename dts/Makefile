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

SOPC2DTS=java -jar sopc2dts.jar
BOARDINFO=${QPFDIR}/${design_name}.sopcinfo ${QPFDIR}/hps_common_board_info.xml board_info.xml

all: ${design_name}.dts 

boot.scr: boot.cmd
	mkimage -C none -A arm -T script -d $< $@

${design_name}.dtb: ${design_name}.dts
	dtc -I dts -O dtb -o $@ -@ $<

${design_name}.dts: ${BOARDINFO} Makefile
	${SOPC2DTS} --input ${QPFDIR}/${design_name}.sopcinfo --output $@ \
	--type dts \
	--board ${QPFDIR}/soc_system_board_info.xml \
	--board ${QPFDIR}/hps_common_board_info.xml \
	--board board_info.xml \
	--bridge-removal all \
	--clocks

clean:
	rm -rf *.dtb *.dts *~

