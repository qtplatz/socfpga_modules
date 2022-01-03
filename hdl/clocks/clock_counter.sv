//-------------------------------------------------------------------------------
//-- Title      : slow clock generator
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017, 2021 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none

module clock_counter ( input wire clk
                       , input wire reset_n
                       , input wire sclk
                       , output wire [WIDTH-1:0] count );

   parameter WIDTH = 32;
   parameter PIPELINE = 4;
   reg [PIPELINE-1:0] sclk_reg = 0;
   reg [WIDTH-1:0]    clock_counter = 0;

   always @(posedge clk) begin
      if ( ~reset_n ) begin
         clock_counter <= 0;
         sclk_reg <= 0;
      end
      else begin
         sclk_reg <= { sclk_reg[ $bits( sclk_reg ) - 2 : 0], sclk };
         if ( sclk_reg[ $bits( sclk_reg ) - 1 : $bits( sclk_reg ) - 2 ] == 2'b01 )
           clock_counter <= clock_counter + 1;
      end
   end

   assign count = clock_counter;

endmodule
