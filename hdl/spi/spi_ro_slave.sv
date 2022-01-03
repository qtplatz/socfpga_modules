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

module spi_ro_slave ( input clk
                      , input wire reset_n
                      , input wire spi_ss_n
                      , input wire spi_sclk
                      , input wire spi_mosi
		      , input wire spi_miso
                      , output wire data_valid                 // dma.sink_valid
                      , output wire [DWIDTH-1:0] master_data
                      , output wire [DWIDTH-1:0] slave_data
                      , output wire [BITCOUNTWIDTH-1:0] nbits_received
                      );

   parameter DWIDTH = 32;
   parameter BITCOUNTWIDTH = 12;
   parameter CPOL = 1;
   parameter CPHA = 0;

   reg [$clog2(DWIDTH)-1:0] bit_counts;
   reg [2:0] 		    sclk, ss_n, mosi, miso; // sclk domain
   reg [DWIDTH-1:0] 	    master_data_r = 0, slave_data_r = 0;
   reg [BITCOUNTWIDTH-1:0]  nbits_received_r = 0;
   reg [DWIDTH-1:0] 	    source_data_reg = 0;

   wire 		    sclk_cpoledge;
   wire 		    ss_n_negedge  = ( ss_n[2:1] == 2'b10 );  // falling edge
   wire 		    ss_n_posedge  = ( ss_n[2:1] == 2'b01 );  // rising edge
   wire 		    ss_active     = ~ss_n[1];
   wire 		    mosi_data     = mosi[1];
   wire 		    miso_data     = miso[1];
   wire 		    sclk_d = sclk[1]; // for debug

   assign master_data = master_data_r;
   assign slave_data = slave_data_r;
   assign nbits_received = nbits_received_r;

   always @( posedge clk ) begin
      sclk <= { sclk[1:0], spi_sclk };
      ss_n <= { ss_n[1:0], spi_ss_n };
      mosi <= { mosi[1:0], spi_mosi };
      miso <= { miso[1:0], spi_miso };
   end

   assign sclk_cpoledge = (CPOL == 1) ? ( sclk[2:1] == 2'b10 ) : ( sclk[2:1] == 2'b01 );
   assign data_valid = ~ss_active;

   always @( posedge clk ) begin
      if ( ~ss_active )
        bit_counts <= 0;
      if ( sclk_cpoledge )
        bit_counts <= bit_counts + 1; // ++bit_counts
   end

   always @( posedge clk ) begin
      if ( sclk_cpoledge ) begin
         if ( bit_counts == 0 ) begin
            master_data_r <= { {(DWIDTH-2){1'b0}}, mosi_data };      // lsb
	    slave_data_r <= { {(DWIDTH-2){1'b0}}, miso_data };      // lsb
         end
         else begin
            master_data_r <= { master_data_r[ DWIDTH-2:0 ], mosi_data }; // big endian
	    slave_data_r <= { slave_data_r[ DWIDTH-2:0 ], miso_data }; // big endian
         end
         nbits_received_r <= bit_counts;
      end
   end

endmodule
