`timescale 1ns / 1ps
`default_nettype none

module spi_test;

   parameter DIV_CLOCK = 4;

   reg         reset_n = 0;
   reg 	       clk100  = 1;
   reg 	       clk1    = 1;
   wire        tsensor_0_clock;
   wire        tsensor_0_ready;
   wire [15:0] tsensor_0_data;
   wire        tsensor_1_ready;
   wire [15:0] tsensor_1_data;

   wire       spi_sclk;
   wire       spi_ss_n;
   wire       spi_mosi;
   wire       spi_miso;

   wire       spi1_sclk;
   wire       spi1_ss_n;
   wire       spi1_mosi;
   wire       spi1_miso;

   parameter CLK_PERIOD = 10.0; // 10ns 100MHz

   initial begin
      $dumpfile("spi_test.vcd");
      $dumpvars(0,spi_test);
      #  0 reset_n = 0;
      #  9.6 reset_n = 1;
      #  90.4;                         // 20ns
      # 500000 $finish;
   end // initial begin

   initial begin
      forever clk100 = #(CLK_PERIOD/2.0) ~clk100;
   end

   /////
   initial begin
      #(CLK_PERIOD - 2)
      forever clk1 = #(CLK_PERIOD*100/2.0) ~clk1;
   end

   //clock_divider #( .COUNT( 100_000_000 ) ) // 2000 ms
   clock_divider #( .COUNT( 100_00 ) ) // 0.2 ms
   tsensor_0_clkgen( .clk( clk100 ), .reset_n( reset_n ), .clock( tsensor_0_clock ) );

   spi_master #( .DWIDTH(16), .CPOL(0), .CPHA(0) )
   tsensor_0( .clk( clk100 )
	      , .reset_n( reset_n )
	      , .sclk( clk1 )
	      , .spi_sclk( spi_sclk ) // output wire spi_sclk
              , .spi_ss_n( spi_ss_n ) // output reg spi_ss_n
              , .spi_miso( spi_miso ) // input spi_miso
              , .spi_mosi( spi_mosi ) // output reg spi_mosi
              , .tx_valid( tsensor_0_clock ) // input tx_valid
              , .tx_ready() // spi can accept next data (read only for this device)
              , .tx_data ( 16'b1010_1010_1010_1011) // spi output data (read only for this device)
              , .rx_valid( tsensor_0_ready ) // output reg rx_valid
              , .rx_data ( tsensor_0_data )  // output reg [DWIDTH-1:0] rx_data
              , .tp() );

   spi_master #( .DWIDTH(16), .CPOL(1), .CPHA(0) )
   tsensor_1( .clk( clk100 )
	      , .reset_n( reset_n )
	      , .sclk( clk1 )
	      , .spi_sclk( spi1_sclk ) // output wire spi_sclk
              , .spi_ss_n( spi1_ss_n ) // output reg spi_ss_n
              , .spi_miso( spi1_miso ) // input spi_miso
              , .spi_mosi( spi1_mosi ) // output reg spi_mosi
              , .tx_valid( tsensor_0_clock ) // input tx_valid
              , .tx_ready() // spi can accept next data (read only for this device)
              , .tx_data ( 16'b1010_1010_1010_1011) // spi output data (read only for this device)
              , .rx_valid( tsensor_1_ready ) // output reg rx_valid
              , .rx_data ( tsensor_1_data )  // output reg [DWIDTH-1:0] rx_data
              , .tp() );

endmodule // test
