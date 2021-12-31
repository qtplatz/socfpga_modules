//-------------------------------------------------------------------------------
//-- Title      : system_top.sv
//-- Project    : CDS inject signal receiver
//-- Authors    : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2016-2021 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

// ============================================================================
// Copyright (c) 2014 by Terasic Technologies Inc.
// ============================================================================
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development
//   Kits made by Terasic.  Other use of this code, including the selling
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use
//   or functionality of this code.
//
// ============================================================================
//
//  Terasic Technologies Inc
//  9F., No.176, Sec.2, Gongdao 5th Rd, East Dist, Hsinchu City, 30070. Taiwan
//
//
//                     web: http://www.terasic.com/
//                     email: support@terasic.com
//
// ============================================================================
//Date:  Tue Dec  2 09:28:38 2014
// ============================================================================

`default_nettype none
`include "../hdl/delay_pulse/delay_width.vh"

`define MODEL_NUMBER    32'h20210801

`define CONFIG_ADC_FIFO
`define ENABLE_HPS
//`define ENABLE_CLK
parameter NDELAY_PAIRS = 9;
parameter NDELAY_PINS  = 7;

typedef struct {
   bit [31:0]  replicates;
   delay_width_t delay_width_pairs[ NDELAY_PAIRS ];
} protocol_t;

enum {
      TSENSOR_0_SCLK    = 0
      , TSENSOR_0_SS_n  = 1
      , TSENSOR_0_MISO  = 2
      , TSENSOR_0_MOSI  = 3
      , I2C_OLED_CLK    = 30
      , I2C_OLED_SDA    = 31
      , INJECT_IN_0     = 34
      , INJECT_IN_1     = 35
      } gpio_1;

module system_top(

            ///////// ADC /////////
            output        ADC_CONVST,
            output        ADC_SCK,
            output        ADC_SDI,
            input         ADC_SDO,

            ///////// ARDUINO /////////
            inout [15:0]  ARDUINO_IO,
            inout         ARDUINO_RESET_N,

`ifdef ENABLE_CLK
            ///////// CLK /////////
            output        CLK_I2C_SCL,
            inout         CLK_I2C_SDA,
`endif /*ENABLE_CLK*/

            ///////// FPGA /////////
            input         FPGA_CLK1_50,
            input         FPGA_CLK2_50,
            input         FPGA_CLK3_50,

            ///////// GPIO /////////
            inout [35:0]  GPIO_0,
            inout [35:0]  GPIO_1,

`ifdef ENABLE_HPS
            ///////// HPS /////////
            inout         HPS_CONV_USB_N,
            output [14:0] HPS_DDR3_ADDR,
            output [2:0]  HPS_DDR3_BA,
            output        HPS_DDR3_CAS_N,
            output        HPS_DDR3_CKE,
            output        HPS_DDR3_CK_N,
            output        HPS_DDR3_CK_P,
            output        HPS_DDR3_CS_N,
            output [3:0]  HPS_DDR3_DM,
            inout [31:0]  HPS_DDR3_DQ,
            inout [3:0]   HPS_DDR3_DQS_N,
            inout [3:0]   HPS_DDR3_DQS_P,
            output        HPS_DDR3_ODT,
            output        HPS_DDR3_RAS_N,
            output        HPS_DDR3_RESET_N,
            input         HPS_DDR3_RZQ,
            output        HPS_DDR3_WE_N,
            output        HPS_ENET_GTX_CLK,
            inout         HPS_ENET_INT_N,
            output        HPS_ENET_MDC,
            inout         HPS_ENET_MDIO,
            input         HPS_ENET_RX_CLK,
            input [3:0]   HPS_ENET_RX_DATA,
            input         HPS_ENET_RX_DV,
            output [3:0]  HPS_ENET_TX_DATA,
            output        HPS_ENET_TX_EN,
            inout         HPS_GSENSOR_INT,
            inout         HPS_I2C0_SCLK,
            inout         HPS_I2C0_SDAT,
            inout         HPS_I2C1_SCLK,
            inout         HPS_I2C1_SDAT,
            inout         HPS_KEY,
            inout         HPS_LED,
            inout         HPS_LTC_GPIO,
            output        HPS_SD_CLK,
            inout         HPS_SD_CMD,
            inout [3:0]   HPS_SD_DATA,
            output        HPS_SPIM_CLK,
            input         HPS_SPIM_MISO,
            output        HPS_SPIM_MOSI,
            inout         HPS_SPIM_SS,
            input         HPS_UART_RX,
            output        HPS_UART_TX,
            input         HPS_USB_CLKOUT,
            inout [7:0]   HPS_USB_DATA,
            input         HPS_USB_DIR,
            input         HPS_USB_NXT,
            output        HPS_USB_STP,
`endif /*ENABLE_HPS*/

            ///////// KEY /////////
            input [1:0]   KEY,

            ///////// LED /////////
            output [7:0]  LED,

            ///////// SW /////////
            input [3:0]   SW

            );


   //=======================================================
   //  REG/WIRE declarations
   //=======================================================
   wire                   hps_fpga_reset_n;
   wire [ 1:0]            fpga_debounced_buttons;
   wire [ 6:0]            fpga_led_internal;
   wire [ 2:0]            hps_reset_req;
   wire                   hps_cold_reset;
   wire                   hps_warm_reset;
   wire                   hps_debug_reset;
   wire [27:0]            stm_hw_events;
   wire 		  fpga_clk_50;
   //
   wire [ 3:0]            pio_out_external_connection_export;
   wire [ 1:0]            pio_in_external_connection_export;
   wire                   pio_dg_external_connection_export;

   wire [ 1:0]            async_inject_in;
   wire [ 1:0]            inject_in;
   wire [ 1:0]            evbox_pulse;

`ifdef CONFIG_ADC_FIFO
   wire [511:0]           adc_fifo_0_st_sink_data;          // time_stamp[64], adc[24*8]
   wire                   adc_fifo_0_st_sink_valid;
   wire                   adc_fifo_0_st_sink_ready;
   wire                   adc_fifo_0_st_sink_startofpacket;
   wire                   adc_fifo_0_st_sink_endofpacket;
   wire [3:0]             adc_fifo_tp;
`endif

   wire                   pll_0_locked;
   wire                   clk100;
   wire                   clk1;

   reg [31:0]             model_number;
   reg [31:0]             revision_number;
   reg [63:0]             timestamp = 64'h0000;
   reg [63:0]             clock_counter = 64'h0000;
   reg [31:0]             t0_counter = 32'h0000;

   reg [31:0]             dg_interval = 'd1000000; // 1 ms default
   wire                   dg_t0;
   wire [31:0]            delay_counter;
   wire [1:0]             user_protocol_number; // setpoint
   reg [ 1:0]             dg_protocol_number_reg;  // actual
   reg [ 31:0]            user_dg_interval;
   reg [ 31:0]            view_dg_interval;
   reg [ 31:0]            user_dg_replicates;
   reg [ 31:0]            view_dg_replicates;
   wire [31:0]            user_flags;
   wire [31:0]            user_commit;
   var protocol_t         user_protocol[ 4 ];
   var delay_width_t      user_delay_width_pairs [ NDELAY_PAIRS ];
   var delay_width_t      view_delay_width_pairs [ NDELAY_PAIRS ];
   var delay_width_t      delay_width_pairs [ NDELAY_PAIRS ];
   wire [NDELAY_PAIRS-1:0] delay_pins; // virtual pins
   wire                    slave_io_0_user_interface_write;
   wire                    slave_io_0_user_interface_read;
   wire [15:0]             slave_io_0_user_interface_chipselect;
   wire [7:0]              slave_io_0_user_interface_byteenable;
   wire                    slave_io_commit_valid;

   wire                    tsensor_data_user_interface_write;
   wire                    tsensor_data_user_interface_read;
   wire [15:0]             tsensor_data_user_interface_chipselect;
   wire [7:0]              tsensor_data_user_interface_byteenable;
   wire                    tsensor_data_commit_valid;

   // tsensor
   reg                     tsensor_0_clock;
   reg [1:0]               tsensor_0_valid;
   reg                     tsensor_0_ready;
   reg [11:0]              tsensor_0_data;
   reg [11:0]              tsensor_0_setpt;

   wire                    tsensor_sync_external_connection_export;

   // connection of internal logics
   assign LED[5:1]         = fpga_led_internal;
   assign LED[7]           = evbox_pulse[1];
   assign LED[6]           = evbox_pulse[0];
   assign fpga_clk_50      = FPGA_CLK1_50;
   assign stm_hw_events    = {{15{1'b0}}, SW, fpga_led_internal, fpga_debounced_buttons};

   assign GPIO_0[  0 ] = ~delay_pins[ 0 ];    // push (pos|neg)
   assign GPIO_0[  1 ] = ~delay_pins[ 1 ];    // injection-sector
   assign GPIO_0[  2 ] = ~( delay_pins[ 2 ] | delay_pins[ 3 ] );  // ejection-sector 0,1
   assign GPIO_0[  3 ] = ~( delay_pins[ 4 ] | delay_pins[ 5 ] );  // gate 0,1
   assign GPIO_0[  4 ] = ~delay_pins[ 6 ];    // adc-delay
   assign GPIO_0[  5 ] = dg_t0;
   // assign GPIO_0[ 7:6] = ( dg_data_io_flags_l & 'h40 ) ? dg_protocol_number_gray : ~dg_protocol_number_gray;

   // output
   assign GPIO_1[ 23: 0 ] = '0;
   assign GPIO_1[ 27:24 ] = pio_out_external_connection_export[ 3:0];
   assign GPIO_1[ 31:28 ] = '0;
   assign GPIO_1[ 33:32 ] = evbox_pulse;                 // LOOP BACK

   // input
   assign async_inject_in[ 1:0 ] = GPIO_1[ 35:34 ];             // external inject in

   assign pio_in_external_connection_export[1:0] = evbox_pulse[1:0];

   assign slave_io_commit_valid
     = slave_io_0_user_interface_write
       & slave_io_0_user_interface_chipselect[15]
       & user_commit[ 0 ];

   //
   //=======================================================
   soc_system u0 (
		  //Clock&Reset
	          .clk_clk                               (FPGA_CLK1_50 ),     //                 clk.clk
	          .reset_reset_n                         (hps_fpga_reset_n ), //                 reset.reset_n
	          //HPS ddr3
	          .memory_mem_a                          ( HPS_DDR3_ADDR),     //                memory.mem_a
	          .memory_mem_ba                         ( HPS_DDR3_BA),       //                .mem_ba
	          .memory_mem_ck                         ( HPS_DDR3_CK_P),     //                .mem_ck
	          .memory_mem_ck_n                       ( HPS_DDR3_CK_N),     //                .mem_ck_n
	          .memory_mem_cke                        ( HPS_DDR3_CKE),      //                .mem_cke
	          .memory_mem_cs_n                       ( HPS_DDR3_CS_N),     //                .mem_cs_n
	          .memory_mem_ras_n                      ( HPS_DDR3_RAS_N),    //                .mem_ras_n
	          .memory_mem_cas_n                      ( HPS_DDR3_CAS_N),    //                .mem_cas_n
	          .memory_mem_we_n                       ( HPS_DDR3_WE_N),     //                .mem_we_n
	          .memory_mem_reset_n                    ( HPS_DDR3_RESET_N),  //                .mem_reset_n
	          .memory_mem_dq                         ( HPS_DDR3_DQ),       //                .mem_dq
	          .memory_mem_dqs                        ( HPS_DDR3_DQS_P),    //                .mem_dqs
	          .memory_mem_dqs_n                      ( HPS_DDR3_DQS_N),    //                .mem_dqs_n
	          .memory_mem_odt                        ( HPS_DDR3_ODT),      //                .mem_odt
	          .memory_mem_dm                         ( HPS_DDR3_DM),       //                .mem_dm
	          .memory_oct_rzqin                      ( HPS_DDR3_RZQ),       //               .oct_rzqin
	          //HPS ethernet
	          .hps_0_hps_io_hps_io_emac1_inst_TX_CLK ( HPS_ENET_GTX_CLK),       //            hps_0_hps_io.hps_io_emac1_inst_TX_CLK
	          .hps_0_hps_io_hps_io_emac1_inst_TXD0   ( HPS_ENET_TX_DATA[0] ),   //            .hps_io_emac1_inst_TXD0
	          .hps_0_hps_io_hps_io_emac1_inst_TXD1   ( HPS_ENET_TX_DATA[1] ),   //            .hps_io_emac1_inst_TXD1
	          .hps_0_hps_io_hps_io_emac1_inst_TXD2   ( HPS_ENET_TX_DATA[2] ),   //            .hps_io_emac1_inst_TXD2
	          .hps_0_hps_io_hps_io_emac1_inst_TXD3   ( HPS_ENET_TX_DATA[3] ),   //            .hps_io_emac1_inst_TXD3
	          .hps_0_hps_io_hps_io_emac1_inst_RXD0   ( HPS_ENET_RX_DATA[0] ),   //            .hps_io_emac1_inst_RXD0
	          .hps_0_hps_io_hps_io_emac1_inst_MDIO   ( HPS_ENET_MDIO ),         //            .hps_io_emac1_inst_MDIO
	          .hps_0_hps_io_hps_io_emac1_inst_MDC    ( HPS_ENET_MDC  ),         //            .hps_io_emac1_inst_MDC
	          .hps_0_hps_io_hps_io_emac1_inst_RX_CTL ( HPS_ENET_RX_DV),         //            .hps_io_emac1_inst_RX_CTL
	          .hps_0_hps_io_hps_io_emac1_inst_TX_CTL ( HPS_ENET_TX_EN),         //            .hps_io_emac1_inst_TX_CTL
	          .hps_0_hps_io_hps_io_emac1_inst_RX_CLK ( HPS_ENET_RX_CLK),        //            .hps_io_emac1_inst_RX_CLK
	          .hps_0_hps_io_hps_io_emac1_inst_RXD1   ( HPS_ENET_RX_DATA[1] ),   //            .hps_io_emac1_inst_RXD1
	          .hps_0_hps_io_hps_io_emac1_inst_RXD2   ( HPS_ENET_RX_DATA[2] ),   //            .hps_io_emac1_inst_RXD2
	          .hps_0_hps_io_hps_io_emac1_inst_RXD3   ( HPS_ENET_RX_DATA[3] ),   //            .hps_io_emac1_inst_RXD3
	          //HPS SD card
	          .hps_0_hps_io_hps_io_sdio_inst_CMD     ( HPS_SD_CMD    ),           //           .hps_io_sdio_inst_CMD
	          .hps_0_hps_io_hps_io_sdio_inst_D0      ( HPS_SD_DATA[0]     ),      //           .hps_io_sdio_inst_D0
	          .hps_0_hps_io_hps_io_sdio_inst_D1      ( HPS_SD_DATA[1]     ),      //           .hps_io_sdio_inst_D1
	          .hps_0_hps_io_hps_io_sdio_inst_CLK     ( HPS_SD_CLK   ),            //           .hps_io_sdio_inst_CLK
	          .hps_0_hps_io_hps_io_sdio_inst_D2      ( HPS_SD_DATA[2]     ),      //           .hps_io_sdio_inst_D2
	          .hps_0_hps_io_hps_io_sdio_inst_D3      ( HPS_SD_DATA[3]     ),      //           .hps_io_sdio_inst_D3
	          //HPS USB
	          .hps_0_hps_io_hps_io_usb1_inst_D0      ( HPS_USB_DATA[0]    ),      //           .hps_io_usb1_inst_D0
	          .hps_0_hps_io_hps_io_usb1_inst_D1      ( HPS_USB_DATA[1]    ),      //           .hps_io_usb1_inst_D1
	          .hps_0_hps_io_hps_io_usb1_inst_D2      ( HPS_USB_DATA[2]    ),      //           .hps_io_usb1_inst_D2
	          .hps_0_hps_io_hps_io_usb1_inst_D3      ( HPS_USB_DATA[3]    ),      //           .hps_io_usb1_inst_D3
	          .hps_0_hps_io_hps_io_usb1_inst_D4      ( HPS_USB_DATA[4]    ),      //           .hps_io_usb1_inst_D4
	          .hps_0_hps_io_hps_io_usb1_inst_D5      ( HPS_USB_DATA[5]    ),      //           .hps_io_usb1_inst_D5
	          .hps_0_hps_io_hps_io_usb1_inst_D6      ( HPS_USB_DATA[6]    ),      //           .hps_io_usb1_inst_D6
	          .hps_0_hps_io_hps_io_usb1_inst_D7      ( HPS_USB_DATA[7]    ),      //           .hps_io_usb1_inst_D7
	          .hps_0_hps_io_hps_io_usb1_inst_CLK     ( HPS_USB_CLKOUT    ),       //           .hps_io_usb1_inst_CLK
	          .hps_0_hps_io_hps_io_usb1_inst_STP     ( HPS_USB_STP    ),          //           .hps_io_usb1_inst_STP
	          .hps_0_hps_io_hps_io_usb1_inst_DIR     ( HPS_USB_DIR    ),          //           .hps_io_usb1_inst_DIR
	          .hps_0_hps_io_hps_io_usb1_inst_NXT     ( HPS_USB_NXT    ),          //           .hps_io_usb1_inst_NXT
		  //HPS SPI
	          .hps_0_hps_io_hps_io_spim1_inst_CLK    ( HPS_SPIM_CLK  ),           //           .hps_io_spim1_inst_CLK
	          .hps_0_hps_io_hps_io_spim1_inst_MOSI   ( HPS_SPIM_MOSI ),           //           .hps_io_spim1_inst_MOSI
	          .hps_0_hps_io_hps_io_spim1_inst_MISO   ( HPS_SPIM_MISO ),           //           .hps_io_spim1_inst_MISO
	          .hps_0_hps_io_hps_io_spim1_inst_SS0    ( HPS_SPIM_SS   ),             //         .hps_io_spim1_inst_SS0
		  //HPS UART
	          .hps_0_hps_io_hps_io_uart0_inst_RX     ( HPS_UART_RX   ),          //            .hps_io_uart0_inst_RX
	          .hps_0_hps_io_hps_io_uart0_inst_TX     ( HPS_UART_TX   ),          //            .hps_io_uart0_inst_TX
		  //HPS I2C1
	          .hps_0_hps_io_hps_io_i2c0_inst_SDA     ( HPS_I2C0_SDAT  ),        //             .hps_io_i2c0_inst_SDA
	          .hps_0_hps_io_hps_io_i2c0_inst_SCL     ( HPS_I2C0_SCLK  ),        //             .hps_io_i2c0_inst_SCL
		  //HPS I2C2
	          .hps_0_hps_io_hps_io_i2c1_inst_SDA     ( HPS_I2C1_SDAT  ),        //             .hps_io_i2c1_inst_SDA
	          .hps_0_hps_io_hps_io_i2c1_inst_SCL     ( HPS_I2C1_SCLK  ),        //             .hps_io_i2c1_inst_SCL
		  //GPIO
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO09  ( HPS_CONV_USB_N ),        //         .hps_io_gpio_inst_GPIO09
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO35  ( HPS_ENET_INT_N ),        //         .hps_io_gpio_inst_GPIO35
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO40  ( HPS_LTC_GPIO   ),        //         .hps_io_gpio_inst_GPIO40
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO53  ( HPS_LED   ),             //         .hps_io_gpio_inst_GPIO53
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO54  ( HPS_KEY   ),             //         .hps_io_gpio_inst_GPIO54
	          .hps_0_hps_io_hps_io_gpio_inst_GPIO61  ( HPS_GSENSOR_INT ),       //         .hps_io_gpio_inst_GPIO61

		  //FPGA Partion
	          .led_pio_external_connection_export    ( fpga_led_internal ),      //    led_pio_external_connection.export
	          .dipsw_pio_external_connection_export  ( SW	),                   //  dipsw_pio_external_connection.export
	          .button_pio_external_connection_export ( fpga_debounced_buttons ), // button_pio_external_connection.export
	          .hps_0_h2f_reset_reset_n               ( hps_fpga_reset_n ),       //                hps_0_h2f_reset.reset_n
	          .hps_0_f2h_cold_reset_req_reset_n      (~hps_cold_reset ),      //       hps_0_f2h_cold_reset_req.reset_n
                  .hps_0_f2h_debug_reset_req_reset_n     (~hps_debug_reset ),     //      hps_0_f2h_debug_reset_req.reset_n
                  .hps_0_f2h_stm_hw_events_stm_hwevents  (stm_hw_events ),  //        hps_0_f2h_stm_hw_events.stm_hwevents
                  .hps_0_f2h_warm_reset_req_reset_n      (~hps_warm_reset )      //       hps_0_f2h_warm_reset_req.reset_n
                  //
                  , .pio_0_external_connection_export      ( pio_out_external_connection_export )
                  , .pio_1_external_connection_export      ( pio_in_external_connection_export )
                  , .pio_dg_external_connection_export     ( pio_dg_external_connection_export )
`ifdef CONFIG_ADC_FIFO
                  , .adc_fifo_0_st_sink_data               ( adc_fifo_0_st_sink_data )            // input  wire [31:0]
                  , .adc_fifo_0_st_sink_valid              ( adc_fifo_0_st_sink_valid )           // input  wire
                  , .adc_fifo_0_st_sink_ready              ( adc_fifo_0_st_sink_ready )           // output wire
                  , .adc_fifo_0_st_sink_startofpacket      ( adc_fifo_0_st_sink_startofpacket )   // input  wire
                  , .adc_fifo_0_st_sink_endofpacket        ( adc_fifo_0_st_sink_endofpacket )     // input  wire
`endif
		  , .pll_0_locked_export                   ( pll_0_locked )
		  , .pll_0_outclk1_clk                     ( clk1 )
                  , .clock_bridge_0_out_clk_clk            ( clk100 )

                  , .slave_io_0_user_interface_dataout_0    ( { 32'b1,                              user_dg_interval } )   // input  wire [63:0]
		  , .slave_io_0_user_interface_dataout_1    ( { user_delay_width_pairs[ 0  ].delay, user_delay_width_pairs[ 0  ].width } )  // push
		  , .slave_io_0_user_interface_dataout_2    ( { user_delay_width_pairs[ 1  ].delay, user_delay_width_pairs[ 1  ].width } )  // inject
		  , .slave_io_0_user_interface_dataout_3    ( { user_delay_width_pairs[ 2  ].delay, user_delay_width_pairs[ 2  ].width } )  // exit_0
		  , .slave_io_0_user_interface_dataout_4    ( { user_delay_width_pairs[ 3  ].delay, user_delay_width_pairs[ 3  ].width } )  // exit_1
		  , .slave_io_0_user_interface_dataout_5    ( { user_delay_width_pairs[ 4  ].delay, user_delay_width_pairs[ 4  ].width } )  // gate_0
		  , .slave_io_0_user_interface_dataout_6    ( { user_delay_width_pairs[ 5  ].delay, user_delay_width_pairs[ 5  ].width } )  // gate_1
		  , .slave_io_0_user_interface_dataout_7    ( { user_delay_width_pairs[ 6  ].delay, user_delay_width_pairs[ 6  ].width } )  // adc_del
		  , .slave_io_0_user_interface_dataout_8    ( { user_delay_width_pairs[ 7  ].delay, user_delay_width_pairs[ 7  ].width } )  // delay_6
		  , .slave_io_0_user_interface_dataout_9    ( { user_delay_width_pairs[ 8  ].delay, user_delay_width_pairs[ 8  ].width } )  // delay_7
		  , .slave_io_0_user_interface_dataout_10   ()
		  , .slave_io_0_user_interface_dataout_11   ()  // output wire [63:0]                                .dataout_11
		  , .slave_io_0_user_interface_dataout_12   ( { 32'b0, user_dg_replicates } )
		  , .slave_io_0_user_interface_dataout_13   ()
		  , .slave_io_0_user_interface_dataout_14   ( { 62'b0, user_protocol_number } )  // output wire [63:0]
		  , .slave_io_0_user_interface_dataout_15   ( { user_flags, user_commit } ) // write only, cannot read
                  // fpga -> hps
		  , .slave_io_0_user_interface_datain_0     ( { user_flags,                         view_dg_interval } )   // input  wire [63:0]
		  , .slave_io_0_user_interface_datain_1     ( { view_delay_width_pairs[ 0  ].delay, view_delay_width_pairs[ 0  ].width } )  // push
                  , .slave_io_0_user_interface_datain_2     ( { view_delay_width_pairs[ 1  ].delay, view_delay_width_pairs[ 1  ].width } )  // inject
                  , .slave_io_0_user_interface_datain_3     ( { view_delay_width_pairs[ 2  ].delay, view_delay_width_pairs[ 2  ].width } )  // exit_0
                  , .slave_io_0_user_interface_datain_4     ( { view_delay_width_pairs[ 3  ].delay, view_delay_width_pairs[ 3  ].width } )  // exit_1
                  , .slave_io_0_user_interface_datain_5     ( { view_delay_width_pairs[ 4  ].delay, view_delay_width_pairs[ 4  ].width } )  // gate_0
                  , .slave_io_0_user_interface_datain_6     ( { view_delay_width_pairs[ 5  ].delay, view_delay_width_pairs[ 5  ].width } )  // gate_1
                  , .slave_io_0_user_interface_datain_7     ( { view_delay_width_pairs[ 6  ].delay, view_delay_width_pairs[ 6  ].width } )  // adc_delay
                  , .slave_io_0_user_interface_datain_8     ( { view_delay_width_pairs[ 7  ].delay, view_delay_width_pairs[ 7  ].width } )  // delay_6
                  , .slave_io_0_user_interface_datain_9     ( { view_delay_width_pairs[ 8  ].delay, view_delay_width_pairs[ 8  ].width } )  // delay_7
                  , .slave_io_0_user_interface_datain_10    ()
		  , .slave_io_0_user_interface_datain_11    ()   // input  wire [63:0]
		  , .slave_io_0_user_interface_datain_12    ( { 32'b0, view_dg_replicates } )
		  , .slave_io_0_user_interface_datain_13    (   timestamp                                             )
		  , .slave_io_0_user_interface_datain_14    ( { t0_counter, 30'b0,             user_protocol_number } )
		  , .slave_io_0_user_interface_datain_15    ( { model_number,                  revision_number } )
                  //
		  , .slave_io_0_user_interface_write        ( slave_io_0_user_interface_write )       // output wire
		  , .slave_io_0_user_interface_read         ( slave_io_0_user_interface_read  )       // output wire
		  , .slave_io_0_user_interface_chipselect   ( slave_io_0_user_interface_chipselect )  // output wire [15:0]
		  , .slave_io_0_user_interface_byteenable   ( slave_io_0_user_interface_byteenable )  // output wire [7:0]
                  //
                  , .tsensor_data_user_interface_dataout_0    ( tsensor_0_setpt )   // input  wire [31:0]
		  , .tsensor_data_user_interface_dataout_1    ()
		  , .tsensor_data_user_interface_dataout_2    ()
		  , .tsensor_data_user_interface_dataout_3    ()
		  , .tsensor_data_user_interface_dataout_4    ()
		  , .tsensor_data_user_interface_dataout_5    ()
		  , .tsensor_data_user_interface_dataout_6    ()
		  , .tsensor_data_user_interface_dataout_7    ()
		  , .tsensor_data_user_interface_dataout_8    ()
		  , .tsensor_data_user_interface_dataout_9    ()
		  , .tsensor_data_user_interface_dataout_10   ()
		  , .tsensor_data_user_interface_dataout_11   ()  // output wire [63:0]                                .dataout_11
		  , .tsensor_data_user_interface_dataout_12   ()
		  , .tsensor_data_user_interface_dataout_13   ()
		  , .tsensor_data_user_interface_dataout_14   ()
		  , .tsensor_data_user_interface_dataout_15   ()
                  // fpga -> hps
		  , .tsensor_data_user_interface_datain_0     ( tsensor_0_setpt ) // setpt
		  , .tsensor_data_user_interface_datain_1     ( tsensor_0_data )  // actual
                  , .tsensor_data_user_interface_datain_2     ()
                  , .tsensor_data_user_interface_datain_3     ()
                  , .tsensor_data_user_interface_datain_4     ()
                  , .tsensor_data_user_interface_datain_5     ()
                  , .tsensor_data_user_interface_datain_6     ()
                  , .tsensor_data_user_interface_datain_7     ()
                  , .tsensor_data_user_interface_datain_8     ()
                  , .tsensor_data_user_interface_datain_9     ()
                  , .tsensor_data_user_interface_datain_10    ()
		  , .tsensor_data_user_interface_datain_11    ()   // input  wire [63:0]
		  , .tsensor_data_user_interface_datain_12    ()
		  , .tsensor_data_user_interface_datain_13    ( t0_counter )
		  , .tsensor_data_user_interface_datain_14    ( model_number )
		  , .tsensor_data_user_interface_datain_15    ( revision_number )
                  //
                  , .tsensor_data_user_interface_write()
                  , .tsensor_data_user_interface_read()
                  , .tsensor_data_user_interface_chipselect()
                  , .tsensor_data_user_interface_byteenable()
                  , .tsensor_data_commit_valid()
                  //
                  , .tsensor_sync_external_connection_export( tsensor_sync_external_connection_export )
                  );

   // Debounce logic to clean out glitches within 1ms
   debounce #(.WIDTH( 2 ), .POLARITY( "LOW" ), .TIMEOUT( 50000 ), .TIMEOUT_WIDTH( 16 ) )            // ceil(log2(TIMEOUT))
   debounce_inst (
                  .clk                                  ( fpga_clk_50 )
                  , .reset_n                            ( hps_fpga_reset_n )
                  , .data_in                            ( KEY )
                  , .data_out                           ( fpga_debounced_buttons )
                  );

   revision #( .MODEL_NUMBER( `MODEL_NUMBER )
               ) revision_0 ( .revision_number ( revision_number )
                              , .model_number  ( model_number )
                              );

   // Source/Probe megawizard instance
   hps_reset hps_reset_inst (
                             .source_clk ( fpga_clk_50 )
                             , .source     ( hps_reset_req )
                             );

   altera_edge_detector #(.PULSE_EXT( 6 ), .EDGE_TYPE( 1 ), .IGNORE_RST_WHILE_BUSY( 1 ) )
   pulse_cold_reset (
                     .clk         ( fpga_clk_50 )
                     , .rst_n     ( hps_fpga_reset_n )
                     , .signal_in ( hps_reset_req[0] )
                     , .pulse_out ( hps_cold_reset )
                     );

   altera_edge_detector #(.PULSE_EXT( 2 ), .EDGE_TYPE( 1 ), .IGNORE_RST_WHILE_BUSY( 1 ) )
   pulse_warm_reset (
                     .clk         ( fpga_clk_50 )
                     , .rst_n     ( hps_fpga_reset_n )
                     , .signal_in ( hps_reset_req[1] )
                     , .pulse_out ( hps_warm_reset )
                     );

   altera_edge_detector #(.PULSE_EXT( 32 ), .EDGE_TYPE( 1 ), .IGNORE_RST_WHILE_BUSY( 1 ) )
   pulse_debug_reset (
                      .clk         ( fpga_clk_50 )
                      , .rst_n     ( hps_fpga_reset_n )
                      , .signal_in ( hps_reset_req[2] )
                      , .pulse_out ( hps_debug_reset)
                      );

   generate
      genvar              i;
      for ( i = 0; i < 2; i = i + 1 ) begin : evbox
         evbox_in #( .N_FF(8), .PULSEWIDTH( 25000000 ) )
         evbox_in_i ( .clk         ( fpga_clk_50 )
                      , .data_in   ( inject_in[ i ] )
                      , .io_port   ( async_inject_in[ i ] )
                      , .pulse_out ( evbox_pulse[ i ] )
                      );
      end
   endgenerate

`ifdef CONFIG_ADC_FIFO
   adc_fifo #( $bits( adc_fifo_0_st_sink_data ) )
   adc_fifo_0 ( .clk                  ( fpga_clk_50 )
                , .reset_n            ( hps_fpga_reset_n )
                , .spi_ss_n           ( ADC_CONVST )
                , .spi_sclk           ( ADC_SCK )
                , .spi_miso           ( ADC_SDO )              // input, 'command to be sent to serial'
                , .spi_mosi           ( ADC_SDI )              // output, return adc data
                , .dipsw              ( SW )
                , .sink_data          ( adc_fifo_0_st_sink_data )  // output
                , .sink_valid         ( adc_fifo_0_st_sink_valid ) // output
                , .sink_ready         ( adc_fifo_0_st_sink_ready ) // input
                , .sink_startofpacket ( adc_fifo_0_st_sink_startofpacket )
                , .sink_endofpacket   ( adc_fifo_0_st_sink_endofpacket )
                , .in_flags           ( { pio_in_external_connection_export, 6'b0} )
                , .out_flags          ( { 20'b0, pio_out_external_connection_export }  )
                , .tp()
                );
`endif

   assign LED [ 0 ] = ~pll_0_locked;

   assign pio_dg_external_connection_export = dg_t0;

   delay_pulse_generator #(.NDELAY_CHANNELS( NDELAY_PAIRS ) )
   delay_pulse_generator_i( .clk                      ( clk100 )
                            , .reset_n                ( hps_fpga_reset_n )
                            , .interval               ( dg_interval )
                            , .user_delay_width_pairs ( user_protocol[ dg_protocol_number_reg ].delay_width_pairs )
                            , .delay_width_pairs      ( delay_width_pairs )
                            , .user_data_valid        ( slave_io_commit_valid )
                            , .pins                   ( delay_pins )
                            , .tp0                    ( dg_t0 )
                            , .delay_counter          ( delay_counter )
                            , .debug()
                            );

   /**********************************************
    *** MULTIPLEXING SLAVE_IO
    * user_flags := input of slave_io_user_interface[ 63:32 ][15] --> output of slave_io_user_intaface[ 63:32][0]
    * user_flags[ 2: 0] assigned to a page number
    * 0..3 := select protocol number where protocol number is a 0..3
    * expose internal actual delay_pulse status to slave_io_user_interface read operation on ARM when user_flags[ 2: 0] == 4
    **/
   wire [ 8: 0 ] slave_io_chipselect;
   wire [ 1: 0 ] user_page;

   assign slave_io_chipselect = slave_io_0_user_interface_chipselect[ 9:1 ];
   assign user_page = user_flags[ 1: 0 ];

   always @* begin
      if ( hps_fpga_reset_n == 0 ) begin
         user_protocol = '{default:'0};
      end
      else if ( slave_io_0_user_interface_write && user_flags[ 2 ] == 0 ) begin
         casex ( slave_io_chipselect )
           9'bxxxxxxxx1: user_protocol[ user_page ].delay_width_pairs[ 0 ] = user_delay_width_pairs[ 0 ];
           9'bxxxxxxx1x: user_protocol[ user_page ].delay_width_pairs[ 1 ] = user_delay_width_pairs[ 1 ];
           9'bxxxxxx1xx: user_protocol[ user_page ].delay_width_pairs[ 2 ] = user_delay_width_pairs[ 2 ];
           9'bxxxxx1xxx: user_protocol[ user_page ].delay_width_pairs[ 3 ] = user_delay_width_pairs[ 3 ];
           9'bxxxx1xxxx: user_protocol[ user_page ].delay_width_pairs[ 4 ] = user_delay_width_pairs[ 4 ];
           9'bxxx1xxxxx: user_protocol[ user_page ].delay_width_pairs[ 5 ] = user_delay_width_pairs[ 5 ];
           9'bxx1xxxxxx: user_protocol[ user_page ].delay_width_pairs[ 6 ] = user_delay_width_pairs[ 6 ];
           9'bx1xxxxxxx: user_protocol[ user_page ].delay_width_pairs[ 7 ] = user_delay_width_pairs[ 7 ];
           9'b1xxxxxxxx: user_protocol[ user_page ].delay_width_pairs[ 8 ] = user_delay_width_pairs[ 8 ];
         endcase
      end // if ( slave_io_0_user_interface_write && user_flags[ 2 ] == 0 )
   end

   assign view_delay_width_pairs = user_flags[ 2 ] ? delay_width_pairs : user_protocol[ user_flags[ 1:0 ] ].delay_width_pairs;
   assign view_dg_interval       = user_flags[ 2 ] ? dg_interval : user_dg_interval;
   assign view_dg_replicates     = user_protocol[ user_flags[ 1:0 ] ].replicates;

   // ------------
   always @( posedge clk100 ) begin
      if ( ~hps_fpga_reset_n ) begin
         dg_interval <= 'd100000; // 10ns * 100k := 1ms
      end
      else if ( slave_io_commit_valid ) begin
         dg_interval <= user_dg_interval < 1000 ? 'd1000 : user_dg_interval; // 1us minimum
      end
   end

   // 100MHz clock counter
   always @( posedge clk100 ) begin
      if ( ~hps_fpga_reset_n ) begin
         clock_counter <= 0;
      end
      else begin
         clock_counter <= clock_counter + 1'b1;
      end
   end

   always @* begin
      if ( dg_t0 ) begin
         timestamp = clock_counter;
         t0_counter = t0_counter + 1;
         dg_protocol_number_reg = user_protocol_number;
      end
   end

   /********************************
    * Temperatur sensor
    *******************************/
   clock_divider #( .COUNT( 500_000 ) )
   tsensor_0_clkgen( clk100, hps_fpga_reset_n, tsensor_0_clock );

   always @* begin
      if ( tsensor_0_ready ) begin
         pio_tsensor_0_external_connection_in_port = tsensor_0_data;
      end
   end

   assign GPIO_1[ 4 ] = clk1;
   assign GPIO_1[ 5 ] = tsensor_0_valid[ 0 ] & ~tsensor_0_valid[ 1 ];

   spi_master #(.DWIDTH(12), .CPOL(1), .CPHA(0) )
   tsensor_0 ( .clk        ( clk100 )
               , .reset_n  ( hps_fpga_reset_n )
               , .sclk     ( clk1 )     // 1 MHz
               , .spi_sclk ( GPIO_1[ TSENSOR_0_SCLK ] ) // output
               , .spi_ss_n ( GPIO_1[ TSENSOR_0_SS_n ] ) // output
               , .spi_miso ( GPIO_1[ TSENSOR_0_MISO ] ) // input
               , .spi_mosi ( GPIO_1[ TSENSOR_0_MOSI ] ) // output
               , .tx_valid ( tsensor_0_clock ) // input trigger
               , .tx_ready () // spi can accept next data (read only for this device)
               , .tx_data  () // spi output data (read only for this device)
               , .rx_valid ( tsensor_0_ready ) // <-- spi
               , .rx_data  ( tsensor_0_data )  // <-- spi
               , .tp()
               );


endmodule
