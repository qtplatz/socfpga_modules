//-------------------------------------------------------------------------------
//-- Title      : ADC adder (averager)
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-- See Table 1. of 'http://cds.linear.com/docs/en/datasheet/2308fc.pdf'
//-------------------------------------------------------------------------------

module adc_adder ( input wire clk
                   , input  wire reset_n
                   , input  wire [2:0] channel           // 0..7
                   , input  wire data_ready
                   , input  wire [11:0] number_of_samples      // 12bit (0..4095) (:= n - 1)
                   , input  wire [11:0] data                   // 12bit adc data
                   , output reg [255:0] accumulated_data  // [64bit timestamp][24bit d0, 24bit d1 ... d7]
                   , output reg data_valid
                   );

   parameter NCHANNELS = 8;

   reg data_ready_d = 0;
   reg [23:0] ax [0:NCHANNELS-1];
   reg [11:0] cx [0:NCHANNELS-1]; //, cx_d[0:NCHANNELS-1];     // nacc = 0..4095 (for 24bit intager from 12bit integer)

   reg [51:0] adc_counter = 0;
   reg [$clog2(NCHANNELS):0] k;

   initial begin
      for ( k = 0; k < NCHANNELS; k = k + 1 ) begin
         ax[k] <= 0;
         cx[k] <= 0;
      end
   end

   always @( posedge data_ready ) begin

      ax[channel] <= ( cx[channel] == 0 ) ? data : ax[channel] + data;
      cx[channel] <= cx[channel] + 1;
      //adc_counter <= adc_counter + 1;

      if ( cx[channel] == number_of_samples ) begin

         cx[channel] <= 0;

         if ( channel == NCHANNELS - 1 ) begin
            adc_counter <= adc_counter + 1;
            accumulated_data[ 24*(7+1)-1 : 24*7 ] <= ax[0]; // pin 0
            accumulated_data[ 24*(6+1)-1 : 24*6 ] <= ax[1]; // pin 1
            accumulated_data[ 24*(5+1)-1 : 24*5 ] <= ax[2]; // pin 2
            accumulated_data[ 24*(4+1)-1 : 24*4 ] <= ax[3]; // pin 3
            accumulated_data[ 24*(3+1)-1 : 24*3 ] <= ax[4]; // pin 4
            accumulated_data[ 24*(2+1)-1 : 24*2 ] <= ax[5]; // pin 5
            accumulated_data[ 24*(1+1)-1 : 24*1 ] <= ax[6]; // pin 6
            accumulated_data[ 24*(0+1)-1 : 24*0 ] <= ax[7]; // pin 7

            accumulated_data[ 243:192 ] <= adc_counter;       // 250kHz counter
            accumulated_data[ 255:244 ] <= number_of_samples; // 0..4095
            data_valid <= 1;  // accumulation complete for all channels -> trigger dma
         end
      end // if ( cx[channel] == number_of_samples )

      if ( data_valid ) begin
        data_valid <= 0; // invalidate accumulated_data
      end
   end

endmodule
