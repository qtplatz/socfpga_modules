//-------------------------------------------------------------------------------
//-- Title      : delay/pulse generator
//-- Project    : MULPIX 8
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (c) 2017-2018 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

module delay_pulse ( input wire clk
                     , input wire reset_n
                     , input wire t0         // must be one clock pulse
                     , input wire [WIDTH-1:0] delay
                     , input wire [WIDTH-1:0] width
                     , output reg pinout
                     );

   parameter WIDTH = 32;

   reg [WIDTH:0] counter [2];
   reg           t0_d;

   /*
   always @* begin
      // pinout will raise one clock later from t0 pulse
      if ( counter[0] != counter[1] )
        pinout = ( counter[0] != 0 ) ^ ( counter[1] != 0 );
   end
    */

   always @( posedge clk or negedge reset_n ) begin
      if ( ~reset_n ) begin
         counter[0] <= 0;
         counter[1] <= 0;
      end
      else begin
         t0_d <= t0;
         if ( t0 & ~t0_d ) begin // 0 -> 1 transition
            counter[0] <= delay;
            counter[1] <= width + delay;
         end

         if ( counter[ 0 ] )
           counter[0] <= counter[0] - 1;

         if ( counter[ 1 ] )
           counter[1] <= counter[1] - 1;

         pinout <= ( counter[0] != 0 ) ^ ( counter[1] != 0 );

      end // else: !if( ~reset_n )
   end // always @ ( posedge clk or negedge reset_n )

endmodule // pulse_counter
