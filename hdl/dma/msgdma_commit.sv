//-------------------------------------------------------------------------------
//-- Title      : mSGDMA stream connector
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none 

module msgdma_commit ( input clk
                       , input reset_n
                       , input data_valid
                       , input [N-1:0] data
                       , output reg [N-1:0] sink_data     // mSGDMA.st_sink_data
                       , output reg sink_valid            // mSGDMA.st_sink_valid
                       , input sink_ready                 // mSGDMA.st_sink_ready
                       , input sink_startofpacket         
                       , input sink_endofpacket
                       );

   parameter N = 32;

   reg data_valid_d;
   reg sink_valid_d;

   always @(posedge clk) begin

      if ( reset_n == 0 ) begin
         sink_valid <= 0;
         data_valid_d <= 0;
         sink_valid_d <= 0;
         sink_valid <= 0;
      end
      else begin
         
         data_valid_d <= data_valid;
         sink_valid_d <= sink_valid;
         
         if ( ~data_valid_d & data_valid ) begin      // 0 -> 1 transition
            sink_data <= data;
            sink_valid <= 1;
         end

         if ( ~sink_valid_d & sink_valid ) begin      // 0 -> 1 transition
            sink_valid <= 0;
         end 
         
      end // else: !if( reset_n == 0 )
   end // always @ (posedge clk)


endmodule


