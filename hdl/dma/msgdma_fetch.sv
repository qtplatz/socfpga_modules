//-------------------------------------------------------------------------------
//-- Title      : mSGDMA stream connector
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none 

module msgdma_fetch ( input clk
                      , input reset_n
                      , output reg [N-1:0] data
                      , output reg data_valid
                      , input [N-1:0] source_data
                      , input source_valid
                      , output reg source_ready           // dma source ready
                      , output reg source_startofpacket
                      , output reg source_endofpacket
                      , input fetch_enable
                      );
   
   parameter N = 32;

   reg source_valid_d = 0;
   reg fetch_enable_d = 0;
   
   always @(posedge clk) begin
      
      if ( reset_n == 0 ) begin
         source_valid_d <= 0;
         source_ready <= 1;
         data_valid <= 0;
      end
      else begin
         source_valid_d <= source_valid;
         if ( ~source_valid_d & source_valid ) begin  // 0 -> 1 transition
            source_ready <= 0;
            data_valid <= 1;
            data <= source_data;                       // fetch data from dma
         end
         else begin
            data_valid <= 0;            
         end
         if ( ~fetch_enable_d & fetch_enable ) begin  // 0 -> 1 transition
            source_ready <= 1;
         end
      end // else: !if( reset_n == 0 )
   end // always @ (posedge clk)

endmodule


