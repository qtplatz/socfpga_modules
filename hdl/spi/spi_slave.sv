//-------------------------------------------------------------------------------
//-- Title      : SPI slave interface
//-- Project    : infiTOF power supply control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2018 MS-Cheminfomatics LLC
// ---------------------------
// CPHA = 0 only supported
// CPOL = [0|1] supported by parameter
// Reference: http://www.fpga4fun.com/SPI2.html
//-------------------------------------------------------------------------------

module spi_slave ( input clk
                   , input wire reset_n
                   , input wire spi_ss_n
                   , input wire spi_sclk
                   , input wire spi_mosi
		   , output wire spi_miso
                   , output wire sink_valid                 // dma.sink_valid
                   , output wire [DWIDTH-1:0] master_in_data
		   , input wire [DWIDTH-1:0] slave_out_data
                   );

   parameter DWIDTH = 32;
   parameter BITCOUNTWIDTH = 12;
   parameter CPOL = 1;
   parameter CPHA = 0;

   reg 			    miso = 0;
   reg [DWIDTH-1:0] 	    tx_data = 0; // slave out data
   reg [DWIDTH-1:0] 	    rx_data = 0; // master in data
   reg [$clog2(DWIDTH):0]   io_counts;
   reg 			    spi_ss_n_d = 1'b1;

   assign spi_miso = miso;
   assign master_in_data = rx_data;

   always @* begin
      if ( spi_ss_n == 0 ) begin
	 tx_data = slave_out_data;
      end
      if ( spi_ss_n == 1 ) begin
	 io_counts = DWIDTH;
      end
      if ( spi_sclk == 1 ) begin
	 miso = tx_data[ io_counts ];
      end
   end

   always @(posedge spi_sclk ) begin
      if ( io_counts ) begin
	 io_counts <= io_counts - 1;
	 if ( io_counts < DWIDTH ) begin
	    rx_data[ io_counts ] <= spi_mosi;
	 end
      end
   end

endmodule
