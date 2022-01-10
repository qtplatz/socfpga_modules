//-------------------------------------------------------------------------------
//-- Title      : SPI master interface
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2018,2021 MS-Cheminfomatics LLC
// ---------------------------
// CPHA = 0 only supported
// CPOL = [0|1] supported by parameter
//-------------------------------------------------------------------------------
`default_nettype none
//------------------------------------------------------
//------------------------------------------------------
//------------------------------------------------------
module spi_master ( input clk
                    , input reset_n
                    , input sclk
		    , output wire spi_sclk
                    , output reg spi_ss_n
                    , input spi_miso
                    , output reg spi_mosi
                    , input tx_valid
                    , output reg tx_ready      // can accept next data
                    , input [DWIDTH-1:0] tx_data
                    , output reg rx_valid
                    , output reg [DWIDTH-1:0] rx_data
                    , output reg tp
                    );

   parameter DWIDTH = 32;
   parameter CPOL = 1;
   parameter CPHA = 0;

   enum { POS, NEG } state_index;

   reg [$clog2(DWIDTH)-1:0] spi_tx_counts;
   reg [DWIDTH-1:0] 	    txd = 0;//, rxd = 0;
   wire 		    latch;
   reg [1:0] 		    latch_d = 0;  // edge {neg,pos}
   reg [1:0] 		    state   = 0;  // edge {neg,pos}
   reg 			    state_d = 0;
   reg 			    spi_sclk_reg = CPOL;

   assign spi_sclk = spi_sclk_reg;

   // 'clk' domain
   reg               tx_valid_d = 0;

   edge_cross_domain #(.POLARITY( "POS" ) )
   cross_domain_0( .aclk        ( clk )
		   , .flag      ( tx_valid )
		   , .bclk      ( sclk )
		   , .sync_out  ( latch ) );

   assign tp = latch;

   // CPHA = 0
   generate
      if ( CPOL == 1 ) begin  // SPI Mode 2  (TI DAC80004)
         always @* begin
            if ( state[ POS ] & state[ NEG ] ) // b11
              spi_sclk_reg = sclk;
            else
              spi_sclk_reg = 1;
            spi_ss_n = ~state[ POS ];
         end
      end
      if ( CPOL == 0 ) begin  // SPI Mode 1 (read data on raising edge) (ex. LTC2308)
         always @* begin
            if ( state == 2'b01 || state == 2'b11 ) // half clock forward
              spi_sclk_reg = ~sclk;
            else
              spi_sclk_reg = 0;
            spi_ss_n = spi_ss_n ? ~state[ NEG ] : ~state[ POS ]; // assert SS_n half clock earlier than data
         end
      end
   endgenerate

   // ------------------------
   // --- DMA clock domain ---
   // ------------------------
   always @( posedge clk ) begin
      tx_valid_d <= tx_valid;
      if ( ~tx_valid_d & tx_valid ) begin     // 0 -> 1 transition
         rx_valid <= 0;
         tx_ready <= 0;
         txd <= tx_data;
      end

      state_d <= state[ POS ]; // state[0]
      if ( state_d && ~state[ POS ] ) begin // 1 -> 0
         tx_ready <= 1;     //
         rx_valid <= 1;      // dma will take over rxd
      end
      if ( rx_valid ) begin
         tx_ready <= 0;     // I'm busy now on
         rx_valid <= 0;
      end
   end

   // ------------------------
   // --- SPI clock domain ---
   // ------------------------
   always @( posedge sclk ) begin
      latch_d[ POS ] <= latch;
      if ( ~latch_d[ POS ] && latch ) begin // 0 -> 1
         spi_tx_counts <= DWIDTH - 1'b1;    // tx nbit remain counter reset
         state[ POS ] <= 1;
	 spi_mosi <= txd [ DWIDTH - 1 ];    // msb data out
      end
      if ( state[ POS ] ) begin
	 if ( spi_tx_counts >= 0 ) begin
	    spi_mosi <= txd [ spi_tx_counts - 1 ];
            if ( spi_tx_counts ) begin
               spi_tx_counts <= spi_tx_counts - 1'b1;
	    end
         end
         state[ POS ] <= spi_tx_counts != 0;
      end
      if ( ~latch && spi_tx_counts == 0 ) begin
	 spi_mosi <= 1'b0;
      end
   end

   always @( negedge sclk ) begin
      latch_d[ NEG ] <= latch;
      if ( ~latch_d[ NEG ] && latch ) // 0 -> 1
        state[ NEG ] <= 1;

      if ( state[ NEG ] & state[ POS ] ) begin  // <--
         rx_data [ DWIDTH-1:0 ] <= { rx_data[ DWIDTH-2:0 ], spi_miso };     // read
         state[ NEG ] <= spi_tx_counts != 0;
      end
   end

endmodule
