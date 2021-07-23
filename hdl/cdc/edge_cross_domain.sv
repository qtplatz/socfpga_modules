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

module edge_cross_domain #(
                            parameter  POLARITY = "POS"
                            )
   (
    input wire aclk
    , input wire flag
    , input wire bclk
    , output wire flagOut // from which we generate a one-clock pulse in clkB domain
    , output wire tp
    );

   reg [1:0]   ack = 0;
   reg [1:0]   flag_d = 0;
   wire         pulse;
   reg          edge_cdc = 0;

   assign tp = pulse;

   always @( posedge aclk ) begin
      flag_d <= { flag_d[0], flag };
   end

   generate
      if ( POLARITY == "POS" ) assign pulse = flag && ( flag_d[1] ^ flag_d[0] );
      if ( POLARITY == "NEG" ) assign pulse = ~flag && ( flag_d[1] ^ flag_d[0] );
   endgenerate

   always @( posedge aclk ) begin
      if ( pulse )
        edge_cdc <= 1'b1;
      if ( ack[1] )
        edge_cdc <= 1'b0;
   end

   always @( posedge bclk ) begin
      ack <= { ack[0], edge_cdc };
   end

   assign sync_out = ~ack[1] & ack[0];

endmodule
