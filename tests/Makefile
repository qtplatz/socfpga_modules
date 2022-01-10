
SRCDIR = ../hdl

TARGETS = spi
all: ${TARGETS}

spi: spi_tb.sv \
	${SRCDIR}/spi/spi_master.sv \
	${SRCDIR}/cdc/edge_cross_domain.sv \
	${SRCDIR}/clocks/clock_divider.sv
	iverilog -DSIMULATION -o $@ -g2005-sv $^
	vvp $@

clean:
	-rm -f ${TARGETS} *.vcd *~
