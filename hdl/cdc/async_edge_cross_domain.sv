//-------------------------------------------------------------------------------
//-- Title      : Clock domain crossing for flag
//-- Project    : ADS54J/Kintex7 high-speed digitiZer+averager
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017-2019 MS-Cheminfomatics LLC
//--
// Reference
// http://www.fpga4fun.com/CrossClockDomain1.html
//-------------------------------------------------------------------------------

//`default_nettype none

module async_edge_cross_domain #(
                                 parameter  POLARITY = "POS"
                                 , parameter DEST_SYNC_FF = 4 // can't be less than 2
                           )
   (
    input wire clk
    , input wire a_flag
    , output wire sync_out // from which we generate a one-clock pulse in clkB domain
    , output wire tp
    );

   reg [DEST_SYNC_FF-1:0] flag_d = 0;
   reg [1:0]              ack = 0;
   wire                   pulse;
   reg                    edge_cdc = 0;

   assign tp = pulse;

   always @( posedge clk ) begin
      flag_d <= { flag_d[ $bits(flag_d) - 2 : 0 ], a_flag };
   end

   generate
      if ( POLARITY == "POS" ) assign pulse = a_flag && ( flag_d[$bits(flag_d)-1] ^ flag_d[$bits(flag_d)-2] );
      if ( POLARITY == "NEG" ) assign pulse = ~a_flag && ( flag_d[$bits(flag_d)-1] ^ flag_d[$bits(flag_d)-2] );
   endgenerate

   always @( posedge clk ) begin
      if ( pulse )
        edge_cdc <= 1'b1;
      if ( ack[1] )
        edge_cdc <= 1'b0;
   end

   always @( posedge clk ) begin
      ack <= { ack[0], edge_cdc };
   end

   assign sync_out = ~ack[1] & ack[0];

endmodule
