//-------------------------------------------------------------------------------
//-- Title      : delay/pulse generator
//-- Project    : delay/pulse generator for MULPIX 8
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (c) 2017-2018 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

`include "delay_width.vh"

module delay_pulse_generator ( input wire clk
                               , input wire reset_n
                               , input wire [WIDTH-1:0] interval
                               , input delay_width_t user_delay_width_pairs [ NDELAY_CHANNELS ]
                               , input wire user_data_valid
                               , output wire [NDELAY_CHANNELS-1:0] pins
                               , output wire tp0
                               , output wire [WIDTH-1:0] delay_counter
                               , output wire [1:0] debug
                               );

   parameter NDELAY_CHANNELS = 9;
   parameter WIDTH = 32;

   reg [1:0] t0;
   reg [WIDTH-1:0] counter = 0;
   reg             user_data_valid_latch, user_data_valid_ack;
   var delay_width_t delay_width_pairs_reg [ NDELAY_CHANNELS ];

   initial begin
      integer k;
      for ( k = 0; k < NDELAY_CHANNELS; k++ ) begin
         delay_width_pairs_reg[ k ].delay = k * 10;
         delay_width_pairs_reg[ k ].width = 10;
      end
   end

   assign delay_counter = counter;
   assign debug = t0;
   assign tp0 = t0[ 0 ];

   always @* begin
      if ( user_data_valid )
        user_data_valid_latch = 1'b1;
      if ( user_data_valid_ack )
        user_data_valid_latch = 1'b0;
   end

   // Master periodic pulse (t0) generation
   always @( posedge clk or negedge reset_n ) begin
      if ( ~reset_n ) begin
         counter <= 0;
         t0 <= 2'b0;
         delay_width_pairs_reg <= user_delay_width_pairs;
         user_data_valid_ack <= 1'b0;
      end
      else begin
         t0[ 1 ] <= t0[ 0 ]; // generate 1 clock pulse

         if ( counter ) begin
            counter <= counter - 1'b1;
            t0[ 0 ] <= 1'b0;
         end
         else if ( counter == 0 ) begin
            counter <= interval;
            t0[ 0 ] <= 1'b1;
            if ( user_data_valid_latch ) begin
               delay_width_pairs_reg <= user_delay_width_pairs;
               user_data_valid_ack <= 1'b1;
            end
         end
         if ( user_data_valid_ack & ~user_data_valid ) begin
           user_data_valid_ack <= 1'b0;
         end
      end
   end // always @ (posedge clk )

   generate
      genvar i;
      for ( i = 0; i < NDELAY_CHANNELS; i = i + 1 ) begin : ctor
         delay_pulse #( .WIDTH(WIDTH) )
         delay_pulse_i( .clk( clk ), .reset_n( reset_n ), .t0(t0), .delay(delay_width_pairs_reg[i].delay), .width(delay_width_pairs_reg[i].width), .pinout( pins[i] ) );
      end
   endgenerate

endmodule // pulse_counter
