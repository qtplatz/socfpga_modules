//-------------------------------------------------------------------------------
//-- Title      : slow clock generator
//-- Project    : TI DAC80004 for MALDI mapping stage control
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

//`default_nettype none

module clock_divider ( input wire clk
                      , input wire reset_n
                      , output wire clock );

   parameter COUNT = 50_000_000;
   parameter WIDTH = $clog2(COUNT);

   reg [WIDTH-1:0] clock_counter = 0;
   reg             clock_reg = 0;
`ifdef XILINX
   BUFG clk_bufg_i( .I( clock_reg ), .O( clock ) );
`else
   assign clock = clock_reg;
`endif
   always @(posedge clk) begin

      if ( reset_n == 0 ) begin
         clock_counter <= 0;
         clock_reg <= 0;
      end else
       begin
         if ( clock_counter == COUNT-1 ) begin
            clock_reg <= ~clock_reg;
            clock_counter <= 0;
         end
         else begin
            clock_counter <= clock_counter + 1;
         end
      end // else: !if( reset_n == 0 )
   end // always @ (posedge clk)

endmodule
