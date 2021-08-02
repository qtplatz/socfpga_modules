//-------------------------------------------------------------------------------
//-- Title      : slow clock generator
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017, 2021 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none

module clock_counter ( input clk, input reset_n, output wire pulse );

   parameter COUNT = 25;

   reg [$clog2(COUNT)-1:0] clock_counter = 0;

   assign pulse = clock_counter == 0; // single clock pulse

   always @(posedge clk) begin

      if ( ~reset_n ) begin
         clock_counter <= 0;
      end
      else begin
         if ( clock_counter == COUNT-1 ) begin
            clock_counter <= 0;
         end
         else begin
            clock_counter <= clock_counter + 1'b1;
         end
      end
   end

endmodule
