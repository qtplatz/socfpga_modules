//-------------------------------------------------------------------------------
//-- Title      : LTC2308 ADC
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017-2018 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

module spi_ltc2308 ( input clk
                    , input reset_n
                    , output reg spi_ss_n
                    , output reg spi_sclk
                    , input spi_miso
                    , output reg spi_mosi
                    , input source_valid
                    , output reg source_ready      // can accept next data
                    , input [DWIDTH-1:0] source_data
                    , output reg sink_valid
                    , output reg [DWIDTH-1:0] sink_data
                    , output reg [1:0] tp
                    );

   parameter DWIDTH = 32;
   parameter CLK = 2;
   parameter CPOL = 1;
   parameter CPHA = 0;
   parameter tACQ = 240 / (20*CLK) + 6 + 1;

   enum { POS, NEG } state_index;

   reg [$clog2(DWIDTH):0] spi_tx_counts;
   reg [$clog2(tACQ):0]   t_acq_counts;

   reg [DWIDTH-1:0]       txd = 0;//, rxd = 0;
   wire              latch;
   reg [1:0]         latch_d = 0;  // edge {neg,pos}
   reg [1:0]         state   = 0;  // edge {neg,pos}
   reg               state_d = 0;
   wire              sclk;
   reg               state_ss = 0;

   // 'clk' domain
   reg               source_valid_d = 0;

   clock_divider #(CLK) spi_clock_0( clk, reset_n, sclk );

   edge_cross_domain #(.POLARITY("POS"))
   cross_domain_0( .aclk( clk ), .flag( source_valid ), .bclk( sclk ), .sync_out( latch ), .tp() );

   always @* begin
      tp[0] = t_acq_counts[0];
      tp[1] = spi_miso;
   end

   // CPHA = 0
   always @* begin
      if ( state == 2'b01 || state == 2'b11 ) // half clock forward
        spi_sclk = ~sclk;
      else
        spi_sclk = 0;
      spi_ss_n = ~( (state[ POS ] | state[ NEG ] ) || t_acq_counts != 0 ); // NEG gives half clock backword timing
   end

   // ------------------------
   // --- DMA clock domain ---
   // ------------------------
   always @( posedge clk ) begin
      source_valid_d <= source_valid;
      if ( ~source_valid_d & source_valid ) begin     // 0 -> 1 transition
         sink_valid <= 0;
         source_ready <= 0;
         txd <= source_data;
      end

      state_d <= state[ POS ];
      if ( state_d && ~state[ POS ] ) begin // 1 -> 0
         source_ready <= 1;     //
         sink_valid <= 1;      // dma will take over rxd
      end
      if ( sink_valid ) begin
         source_ready <= 0;     // I'm busy now on
         sink_valid <= 0;
      end
   end

   // ------------------------
   // --- SPI clock domain ---
   // ------------------------
   always @( posedge sclk ) begin
      latch_d[ POS ] <= latch;
      if ( ~latch_d[ POS ] && latch ) begin // 0 -> 1
         spi_tx_counts <= DWIDTH - 1'b1;
         t_acq_counts <= ( DWIDTH > tACQ ) ? DWIDTH + 2 : tACQ;
         state[ POS ] <= 1;
	 spi_mosi <= txd [ DWIDTH - 1 ];
      end
      if ( state[ POS ] ) begin
	 if ( spi_tx_counts >= 0 ) begin
	    spi_mosi <= txd [ spi_tx_counts - 1 ];
            if ( spi_tx_counts )
              spi_tx_counts <= spi_tx_counts - 1'b1;
         end
         state[ POS ] <= spi_tx_counts != 0;
      end
      if ( t_acq_counts != 0 )
        t_acq_counts <= t_acq_counts - 1'b1;
   end

   always @( negedge sclk ) begin
      latch_d[ NEG ] <= latch;
      if ( ~latch_d[ NEG ] && latch ) // 0 -> 1
        state[ NEG ] <= 1;

      if ( state[ NEG ] & state[ POS ] ) begin  // <--
         sink_data [ DWIDTH-1:0 ] <= { sink_data[ DWIDTH-2:0 ], spi_miso };     // read
         state[ NEG ] <= spi_tx_counts != 0;
      end
   end

endmodule


module adc_ltc2308 ( input clk
                     , input reset_n
                     , output wire spi_ss_n
                     , output wire spi_sclk
                     , input spi_miso
                     , output wire spi_mosi
                     , input adc_clock
                     , input [11:0] number_of_samples
                     , output wire [255:0] accumulated_data
                     , output wire accumulated_data_valid
                     , output wire [1:0] tp_spi
                     );

   parameter DWIDTH=12;
   parameter CLK=2;

   reg [5:0] config_cmd = { 4'o17, 2'o0 }; // sleep mode
   reg [2:0] channel = 3'o7;
   wire      adc_data_ready;  // --> for data adder
   wire [DWIDTH-1:0] adc_data;
   wire              adc_valid;

`define UNI_MODE		1'b1   //1: Unipolar, 0:Bipolar
`define SLP_MODE		1'b0   //1: enable sleep

   always @(posedge adc_clock) begin
      channel <= channel + 1;
   end

   always @(negedge adc_clock)  begin

      case (channel)
	0 : config_cmd <= { 4'o11, 2'o2 }; // 0x9 even,1 (pin 2)
	1 : config_cmd <= { 4'o15, 2'o2 }; // 0xd odd,1  (pin 3)
	2 : config_cmd <= { 4'o12, 2'o2 }; // 0xa even,2 (pin 4)
	3 : config_cmd <= { 4'o16, 2'o2 }; // 0xe odd,2  (pin 5)
	4 : config_cmd <= { 4'o13, 2'o2 }; // 0xb even,3 (pin 6)
	5 : config_cmd <= { 4'o17, 2'o2 }; // 0xf odd,3  (pin 7)
        6 : config_cmd <= { 4'o10, 2'o2 }; // 0x8 even,0 (pin 0)
	7 : config_cmd <= { 4'o14, 2'o2 }; // 0xc odd,0  (pin 1)
	default : config_cmd <= {4'o17, 2'o2}; // sleep mode
      endcase
   end // always @ (negedge adc_clock)

   spi_ltc2308 #(.DWIDTH(DWIDTH), .CLK(CLK) )  // SPI Mode 1
   spi_ltc2308_0 ( .clk( clk )
                   , .reset_n( reset_n )
                   , .spi_ss_n( spi_ss_n ) //ADC_CONVST
                   , .spi_sclk( spi_sclk ) //ADC_SCK
                   , .spi_miso( spi_miso ) //ADC_SDO        // ADC -> FPGA (input)
                   , .spi_mosi( spi_mosi ) //ADC_SDI        // FPGA -> ADC (output)
                   , .source_valid( adc_clock ) // ( spi_txd_valid )
                   , .source_ready( adc_data_ready ) // -> next data can be handled := data is ready
                   , .source_data( { config_cmd, 6'o00 } )
                   , .sink_valid( adc_valid )
                   , .sink_data( adc_data )
                   , .tp( tp_spi ) // mosi, latch
                   );

   adc_adder #(.NCHANNELS(8))
   adc_adder_0 ( .clk( clk )
                 , .reset_n( reset_n )
                 , .channel( channel )
                 , .data_ready( adc_data_ready ) // adc_adc_valid|adc_data_ready
                 , .number_of_samples( number_of_samples )
                 , .data( adc_data )
                 , .accumulated_data( accumulated_data )  // 256bit data
                 , .data_valid( accumulated_data_valid )
                 );


endmodule // adc_ltc2308
