# Makefile
ifeq (${OS},Windows)
	quartus_dir := $(lastword $(wildcard C:/*/*/quartus))
else
	quartus_dir := $(lastword $(wildcard /opt/intelFPGA_lite/20.1/quartus))
endif

project_name = soc_system
design_name  = soc_system
qsys_name    = soc_system
output_dir   = ./soc_system
qsys-generate=${quartus_dir}/sopc_builder/bin/qsys-generate
quartus_sh   =${quartus_dir}/bin/quartus_sh

SOURCES = system_top.sv

${project_name}.sof: ${SOURCES} ${output_dir}/synthesis/${qsys_name}.qip ${qsys_name}.sopcinfo
	${quartus_sh} --flow compile ${project_name} -c ${design_name}

${output_dir}/synthesis/${qsys_name}.qip: ${qsys_name}.qsys revision.v ${project_name}.qsf
	${qsys-generate} ${qsys_name}.qsys --synthesis=VERILOG --output-directory=${output_dir} --family="Cyclone V" --part=5CSEMA4U23C6

${output_dir}/${qsys_name}.bsf: ${qsys_name}.qsys
	${qsys-generate} ${qsys_name}.qsys --block-symbol-file --output-directory=${output_dir} --family="Cyclone V" --part=5CSEMA4U23C6

${qsys_name}.sopcinfo: ${output_dir}/synthesis/${qsys_name}.qip

revision.v:
	${quartus_sh} -t revision.tcl

.PHONY: revision.v
.PHONY: revision
.PHONY: qsys

qsys:
	${qsys-generate} ${qsys_name}.qsys --synthesis=VERILOG --output-directory=${output_dir} --family="Cyclone V" --part=5CSEMA4U23C6

revision:
	${quartus_sh} -t revision.tcl

quartus:
	${quartus_dir}/bin/quartus &

clean:
	rm -f ${output_dir}/synthesis/*.qip ./*.sof

##################################################################
#quartus_map=${quartus_dir}/bin/quartus_map
#quartus_fit=${quartus_dir}/bin/quartus_fit
#quartus_asm=${quartus_dir}/bin/quartus_asm
#quartus_drc=${quartus_dir}/bin/quartus_drc
#quartus_sta=${quartus_dir}/bin/quartus_sta
#output_dir=./soc_system

#soc_system.sof: ghrd.v ${output_dir}/synthesis/soc_system.qip
#	${quartus_map} --read_settings_files=on --write_settings_files=off soc_system -c ${design_name}
#	${quartus_fit} --read_settings_files=off --write_settings_files=off ${design_name} -c soc_system
#	${quartus_asm} --read_settings_files=off --write_settings_files=off ${design_name} -c ${design_name}
#	${quartus_sta} ${design_name} -c ${design_name}
#	${quartus_drc} --read_settings_files=off --write_settings_files=off ${design_name} -c ${design_name}

#${output_dir}/synthesis/soc_system.qip: ${output_dir}/soc_system.bsf soc_system.qsys
#	${qsys-generate} soc_system.qsys --synthesis=VERILOG --output-directory=${output_dir} --family="Cyclone V" --part=5CSEMA4U23C6

#${output_dir}/soc_system.bsf: soc_system.qsys
#	${qsys-generate} soc_system.qsys --block-symbol-file --output-directory=${output_dir} --family="Cyclone V" --part=5CSEMA4U23C6

#clean:
#	rm -f ${output_dir}/synthesis/soc_system.qip soc_system.sof
