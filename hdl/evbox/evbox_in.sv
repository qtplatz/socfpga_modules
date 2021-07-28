//-------------------------------------------------------------------------------
//-- Title      : LC/GC Event IO Box
//-- Project    : delay/pulse generator for infiTOF
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`default_nettype none

module evbox_in (
                 input wire clk
                 , output wire data_in
                 , input wire io_port
                 , output wire pulse_out
                 );

   parameter N_FF = 4;
   parameter PULSEWIDTH = 25000000;

   reg [ N_FF-1:0 ]         ff_reg = '1; // N_FF bits of 1
   reg [ 25 : 0 ]           counter = 0;

   assign pulse_out = |counter;

   always @( posedge clk ) begin
      ff_reg <= { ff_reg[ N_FF - 2 : 0 ], io_port };
      data_in <= ff_reg[ N_FF - 1 ];
   end

   always @( posedge clk ) begin
      if ( counter == 0 && ff_reg[ N_FF - 1 : N_FF - 3 ] == 3'b100) begin
         counter <= PULSEWIDTH;
      end
      if ( counter ) begin
         counter <= counter - 1;
      end
   end

endmodule
