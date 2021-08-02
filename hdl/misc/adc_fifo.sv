//-------------------------------------------------------------------------------
//-- Title      : ADC_FIFO
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017-2018 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none

module adc_fifo( input wire clk
                 , input wire reset_n
                 , output wire spi_ss_n
                 , output wire spi_sclk
                 , input wire spi_miso
                 , output wire spi_mosi
                 , input [3:0] dipsw
                 , output wire [BUSWIDTH-1:0] sink_data     // mSGDMA.st_sink_data
                 , output wire sink_valid                   // mSGDMA.st_sink_valid
                 , input wire sink_ready                    // mSGDMA.st_sink_ready
                 , input wire sink_startofpacket
                 , input wire sink_endofpacket
                 , input wire [7:0] in_flags
                 , input wire [23:0] out_flags
                 , input wire [63:0] cmd_addr
                 , input wire [63:0] cmd_data
                 , input wire cmd_valid
                 , output wire cmd_enable
                 , output wire [3:0] tp
                 );

   parameter BUSWIDTH = 512;

   wire accumulated_data_valid;
   wire [255:0] accumulated_data;
   wire [255:0] meta_data;
   reg [11:0]   adc_number_of_samples = 'd4095;
   reg [63:0]   flags_latch_tp = 0;
   reg [63:0]   clock_counter = 0;
   wire [31:0]  adc_flags;
   wire         adc_clock;
   reg [7:0]    in_flags_d[3] = '{ 0, 0, 0 };
   reg [1:0]    inject_flag_d;
   reg [1:0]    marker_flag_d;
   reg          sink_valid_d, adc_flag_marker, adc_flag_inject;
   reg          cmd_valid_d = 0;
   reg [7:0]    cmd_addr_latch = 0;
   reg [63:0]   cmd_data_latch = 0;

   assign adc_flags = { 2'b0, adc_flag_marker, adc_flag_inject, 4'b0, in_flags_d[0][7:0], out_flags[15:0] };

   always @( posedge clk ) begin
      adc_number_of_samples <= 13'd2**dipsw - 1; // 0..4095 dipsw
   end

   always @( posedge clk ) begin
      if ( ~reset_n ) begin
         clock_counter <= 0;
         sink_valid_d <= 0;
         adc_flag_marker <= 0;
         adc_flag_inject <= 0;
      end
      else begin
         clock_counter <= clock_counter + 1'b1;
         in_flags_d    <= '{ in_flags_d[ 1 ], in_flags_d[ 2 ], in_flags };
         inject_flag_d <= { inject_flag_d[0], in_flags_d[0][0:0] };
         marker_flag_d <= { marker_flag_d[0], in_flags_d[0][1:1] };

         if  ( inject_flag_d == 2'b10 ) begin // 1 -> 0
            adc_flag_inject <= 1'b1;
            flags_latch_tp <= clock_counter;
         end

         sink_valid_d <= sink_valid;

         if ( marker_flag_d == 2'b10 ) begin // 1 -> 0
            adc_flag_marker <= 1'b1;
         end

         if ( sink_valid & ~sink_valid_d ) begin
            adc_flag_inject <= 1'b0;
            adc_flag_marker <= 1'b0;
         end

      end
   end // always @ ( posedge clk )

   always @( posedge clk ) begin
      cmd_valid_d <= cmd_valid;
      if ( cmd_valid & cmd_valid_d ) begin
         cmd_addr_latch <= cmd_addr;
         cmd_data_latch <= cmd_data;
      end
   end

   assign meta_data = { clock_counter, flags_latch_tp, adc_flags, cmd_data_latch[31:0], 64'b0 };

   // ADC cycle clock (max 500kHz)
   clock_divider #( .COUNT(80) ) // 50MHz/(80*2) --> 312.5kHz
   adc_interval( .clk( clk ), .reset_n( reset_n ), .clock( adc_clock ) );

   adc_ltc2308 #( .CLK(2) )
   adc_ltc2308_0 ( .clk                      ( clk       )
                   , .reset_n                ( reset_n   )
                   , .spi_ss_n               ( spi_ss_n  )
                   , .spi_sclk               ( spi_sclk  )
                   , .spi_miso               ( spi_miso  )            // input, 'command to be sent to serial'
                   , .spi_mosi               ( spi_mosi  )            // output, return adc data
                   , .adc_clock              ( adc_clock )            // adc start (command ready)
                   , .number_of_samples      ( adc_number_of_samples )
                   , .accumulated_data       ( accumulated_data )
                   , .accumulated_data_valid ( accumulated_data_valid )
                   );

   //assign tp_dma = accumulated_data_valid;
   assign tp[0] = sink_valid;   //sink_startofpacket | sink_endofpacket;
   assign tp[1] = sink_ready;
   assign tp[2] = adc_clock;
   assign tp[3] = spi_miso;

   msgdma_commit #(BUSWIDTH)
   adc_dma_sender_0 ( .clk( clk )
                      , .reset_n            ( reset_n )
                      , .data_valid         ( accumulated_data_valid )     // <= input, ready to send data back to cpu (replace with rxd_valid)
                      , .data               ( { accumulated_data, meta_data } )  // <= input (data)
                      , .sink_data          ( sink_data )   // output
                      , .sink_valid         ( sink_valid ) // output
                      , .sink_ready         ( sink_ready ) // input
                      , .sink_startofpacket ( sink_startofpacket )
                      , .sink_endofpacket   ( sink_endofpacket )
                      );

endmodule
