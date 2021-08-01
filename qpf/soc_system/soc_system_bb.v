
module soc_system (
	adc_fifo_0_st_sink_data,
	adc_fifo_0_st_sink_valid,
	adc_fifo_0_st_sink_ready,
	adc_fifo_0_st_sink_startofpacket,
	adc_fifo_0_st_sink_endofpacket,
	adc_fifo_0_st_sink_empty,
	button_pio_external_connection_export,
	clk_clk,
	clock_bridge_0_out_clk_clk,
	dipsw_pio_external_connection_export,
	hps_0_f2h_cold_reset_req_reset_n,
	hps_0_f2h_debug_reset_req_reset_n,
	hps_0_f2h_stm_hw_events_stm_hwevents,
	hps_0_f2h_warm_reset_req_reset_n,
	hps_0_h2f_reset_reset_n,
	hps_0_hps_io_hps_io_emac1_inst_TX_CLK,
	hps_0_hps_io_hps_io_emac1_inst_TXD0,
	hps_0_hps_io_hps_io_emac1_inst_TXD1,
	hps_0_hps_io_hps_io_emac1_inst_TXD2,
	hps_0_hps_io_hps_io_emac1_inst_TXD3,
	hps_0_hps_io_hps_io_emac1_inst_RXD0,
	hps_0_hps_io_hps_io_emac1_inst_MDIO,
	hps_0_hps_io_hps_io_emac1_inst_MDC,
	hps_0_hps_io_hps_io_emac1_inst_RX_CTL,
	hps_0_hps_io_hps_io_emac1_inst_TX_CTL,
	hps_0_hps_io_hps_io_emac1_inst_RX_CLK,
	hps_0_hps_io_hps_io_emac1_inst_RXD1,
	hps_0_hps_io_hps_io_emac1_inst_RXD2,
	hps_0_hps_io_hps_io_emac1_inst_RXD3,
	hps_0_hps_io_hps_io_sdio_inst_CMD,
	hps_0_hps_io_hps_io_sdio_inst_D0,
	hps_0_hps_io_hps_io_sdio_inst_D1,
	hps_0_hps_io_hps_io_sdio_inst_CLK,
	hps_0_hps_io_hps_io_sdio_inst_D2,
	hps_0_hps_io_hps_io_sdio_inst_D3,
	hps_0_hps_io_hps_io_usb1_inst_D0,
	hps_0_hps_io_hps_io_usb1_inst_D1,
	hps_0_hps_io_hps_io_usb1_inst_D2,
	hps_0_hps_io_hps_io_usb1_inst_D3,
	hps_0_hps_io_hps_io_usb1_inst_D4,
	hps_0_hps_io_hps_io_usb1_inst_D5,
	hps_0_hps_io_hps_io_usb1_inst_D6,
	hps_0_hps_io_hps_io_usb1_inst_D7,
	hps_0_hps_io_hps_io_usb1_inst_CLK,
	hps_0_hps_io_hps_io_usb1_inst_STP,
	hps_0_hps_io_hps_io_usb1_inst_DIR,
	hps_0_hps_io_hps_io_usb1_inst_NXT,
	hps_0_hps_io_hps_io_spim1_inst_CLK,
	hps_0_hps_io_hps_io_spim1_inst_MOSI,
	hps_0_hps_io_hps_io_spim1_inst_MISO,
	hps_0_hps_io_hps_io_spim1_inst_SS0,
	hps_0_hps_io_hps_io_uart0_inst_RX,
	hps_0_hps_io_hps_io_uart0_inst_TX,
	hps_0_hps_io_hps_io_i2c0_inst_SDA,
	hps_0_hps_io_hps_io_i2c0_inst_SCL,
	hps_0_hps_io_hps_io_i2c1_inst_SDA,
	hps_0_hps_io_hps_io_i2c1_inst_SCL,
	hps_0_hps_io_hps_io_gpio_inst_GPIO09,
	hps_0_hps_io_hps_io_gpio_inst_GPIO35,
	hps_0_hps_io_hps_io_gpio_inst_GPIO40,
	hps_0_hps_io_hps_io_gpio_inst_GPIO53,
	hps_0_hps_io_hps_io_gpio_inst_GPIO54,
	hps_0_hps_io_hps_io_gpio_inst_GPIO61,
	led_pio_external_connection_export,
	memory_mem_a,
	memory_mem_ba,
	memory_mem_ck,
	memory_mem_ck_n,
	memory_mem_cke,
	memory_mem_cs_n,
	memory_mem_ras_n,
	memory_mem_cas_n,
	memory_mem_we_n,
	memory_mem_reset_n,
	memory_mem_dq,
	memory_mem_dqs,
	memory_mem_dqs_n,
	memory_mem_odt,
	memory_mem_dm,
	memory_oct_rzqin,
	pio_0_external_connection_export,
	pio_1_external_connection_export,
	pll_0_locked_export,
	pll_0_outclk1_clk,
	reset_reset_n,
	slave_io_0_user_interface_dataout_0,
	slave_io_0_user_interface_dataout_1,
	slave_io_0_user_interface_dataout_2,
	slave_io_0_user_interface_dataout_3,
	slave_io_0_user_interface_dataout_4,
	slave_io_0_user_interface_dataout_5,
	slave_io_0_user_interface_dataout_6,
	slave_io_0_user_interface_dataout_7,
	slave_io_0_user_interface_dataout_8,
	slave_io_0_user_interface_dataout_9,
	slave_io_0_user_interface_dataout_10,
	slave_io_0_user_interface_dataout_11,
	slave_io_0_user_interface_dataout_12,
	slave_io_0_user_interface_dataout_13,
	slave_io_0_user_interface_dataout_14,
	slave_io_0_user_interface_dataout_15,
	slave_io_0_user_interface_datain_0,
	slave_io_0_user_interface_datain_1,
	slave_io_0_user_interface_datain_2,
	slave_io_0_user_interface_datain_3,
	slave_io_0_user_interface_datain_4,
	slave_io_0_user_interface_datain_5,
	slave_io_0_user_interface_datain_6,
	slave_io_0_user_interface_datain_7,
	slave_io_0_user_interface_datain_8,
	slave_io_0_user_interface_datain_9,
	slave_io_0_user_interface_datain_10,
	slave_io_0_user_interface_datain_11,
	slave_io_0_user_interface_datain_12,
	slave_io_0_user_interface_datain_13,
	slave_io_0_user_interface_datain_14,
	slave_io_0_user_interface_datain_15,
	slave_io_0_user_interface_write,
	slave_io_0_user_interface_read,
	slave_io_0_user_interface_chipselect,
	slave_io_0_user_interface_byteenable);	

	input	[511:0]	adc_fifo_0_st_sink_data;
	input		adc_fifo_0_st_sink_valid;
	output		adc_fifo_0_st_sink_ready;
	input		adc_fifo_0_st_sink_startofpacket;
	input		adc_fifo_0_st_sink_endofpacket;
	input	[5:0]	adc_fifo_0_st_sink_empty;
	input	[3:0]	button_pio_external_connection_export;
	input		clk_clk;
	output		clock_bridge_0_out_clk_clk;
	input	[3:0]	dipsw_pio_external_connection_export;
	input		hps_0_f2h_cold_reset_req_reset_n;
	input		hps_0_f2h_debug_reset_req_reset_n;
	input	[27:0]	hps_0_f2h_stm_hw_events_stm_hwevents;
	input		hps_0_f2h_warm_reset_req_reset_n;
	output		hps_0_h2f_reset_reset_n;
	output		hps_0_hps_io_hps_io_emac1_inst_TX_CLK;
	output		hps_0_hps_io_hps_io_emac1_inst_TXD0;
	output		hps_0_hps_io_hps_io_emac1_inst_TXD1;
	output		hps_0_hps_io_hps_io_emac1_inst_TXD2;
	output		hps_0_hps_io_hps_io_emac1_inst_TXD3;
	input		hps_0_hps_io_hps_io_emac1_inst_RXD0;
	inout		hps_0_hps_io_hps_io_emac1_inst_MDIO;
	output		hps_0_hps_io_hps_io_emac1_inst_MDC;
	input		hps_0_hps_io_hps_io_emac1_inst_RX_CTL;
	output		hps_0_hps_io_hps_io_emac1_inst_TX_CTL;
	input		hps_0_hps_io_hps_io_emac1_inst_RX_CLK;
	input		hps_0_hps_io_hps_io_emac1_inst_RXD1;
	input		hps_0_hps_io_hps_io_emac1_inst_RXD2;
	input		hps_0_hps_io_hps_io_emac1_inst_RXD3;
	inout		hps_0_hps_io_hps_io_sdio_inst_CMD;
	inout		hps_0_hps_io_hps_io_sdio_inst_D0;
	inout		hps_0_hps_io_hps_io_sdio_inst_D1;
	output		hps_0_hps_io_hps_io_sdio_inst_CLK;
	inout		hps_0_hps_io_hps_io_sdio_inst_D2;
	inout		hps_0_hps_io_hps_io_sdio_inst_D3;
	inout		hps_0_hps_io_hps_io_usb1_inst_D0;
	inout		hps_0_hps_io_hps_io_usb1_inst_D1;
	inout		hps_0_hps_io_hps_io_usb1_inst_D2;
	inout		hps_0_hps_io_hps_io_usb1_inst_D3;
	inout		hps_0_hps_io_hps_io_usb1_inst_D4;
	inout		hps_0_hps_io_hps_io_usb1_inst_D5;
	inout		hps_0_hps_io_hps_io_usb1_inst_D6;
	inout		hps_0_hps_io_hps_io_usb1_inst_D7;
	input		hps_0_hps_io_hps_io_usb1_inst_CLK;
	output		hps_0_hps_io_hps_io_usb1_inst_STP;
	input		hps_0_hps_io_hps_io_usb1_inst_DIR;
	input		hps_0_hps_io_hps_io_usb1_inst_NXT;
	output		hps_0_hps_io_hps_io_spim1_inst_CLK;
	output		hps_0_hps_io_hps_io_spim1_inst_MOSI;
	input		hps_0_hps_io_hps_io_spim1_inst_MISO;
	output		hps_0_hps_io_hps_io_spim1_inst_SS0;
	input		hps_0_hps_io_hps_io_uart0_inst_RX;
	output		hps_0_hps_io_hps_io_uart0_inst_TX;
	inout		hps_0_hps_io_hps_io_i2c0_inst_SDA;
	inout		hps_0_hps_io_hps_io_i2c0_inst_SCL;
	inout		hps_0_hps_io_hps_io_i2c1_inst_SDA;
	inout		hps_0_hps_io_hps_io_i2c1_inst_SCL;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO09;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO35;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO40;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO53;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO54;
	inout		hps_0_hps_io_hps_io_gpio_inst_GPIO61;
	output	[7:0]	led_pio_external_connection_export;
	output	[14:0]	memory_mem_a;
	output	[2:0]	memory_mem_ba;
	output		memory_mem_ck;
	output		memory_mem_ck_n;
	output		memory_mem_cke;
	output		memory_mem_cs_n;
	output		memory_mem_ras_n;
	output		memory_mem_cas_n;
	output		memory_mem_we_n;
	output		memory_mem_reset_n;
	inout	[31:0]	memory_mem_dq;
	inout	[3:0]	memory_mem_dqs;
	inout	[3:0]	memory_mem_dqs_n;
	output		memory_mem_odt;
	output	[3:0]	memory_mem_dm;
	input		memory_oct_rzqin;
	output	[3:0]	pio_0_external_connection_export;
	input	[1:0]	pio_1_external_connection_export;
	output		pll_0_locked_export;
	output		pll_0_outclk1_clk;
	input		reset_reset_n;
	output	[63:0]	slave_io_0_user_interface_dataout_0;
	output	[63:0]	slave_io_0_user_interface_dataout_1;
	output	[63:0]	slave_io_0_user_interface_dataout_2;
	output	[63:0]	slave_io_0_user_interface_dataout_3;
	output	[63:0]	slave_io_0_user_interface_dataout_4;
	output	[63:0]	slave_io_0_user_interface_dataout_5;
	output	[63:0]	slave_io_0_user_interface_dataout_6;
	output	[63:0]	slave_io_0_user_interface_dataout_7;
	output	[63:0]	slave_io_0_user_interface_dataout_8;
	output	[63:0]	slave_io_0_user_interface_dataout_9;
	output	[63:0]	slave_io_0_user_interface_dataout_10;
	output	[63:0]	slave_io_0_user_interface_dataout_11;
	output	[63:0]	slave_io_0_user_interface_dataout_12;
	output	[63:0]	slave_io_0_user_interface_dataout_13;
	output	[63:0]	slave_io_0_user_interface_dataout_14;
	output	[63:0]	slave_io_0_user_interface_dataout_15;
	input	[63:0]	slave_io_0_user_interface_datain_0;
	input	[63:0]	slave_io_0_user_interface_datain_1;
	input	[63:0]	slave_io_0_user_interface_datain_2;
	input	[63:0]	slave_io_0_user_interface_datain_3;
	input	[63:0]	slave_io_0_user_interface_datain_4;
	input	[63:0]	slave_io_0_user_interface_datain_5;
	input	[63:0]	slave_io_0_user_interface_datain_6;
	input	[63:0]	slave_io_0_user_interface_datain_7;
	input	[63:0]	slave_io_0_user_interface_datain_8;
	input	[63:0]	slave_io_0_user_interface_datain_9;
	input	[63:0]	slave_io_0_user_interface_datain_10;
	input	[63:0]	slave_io_0_user_interface_datain_11;
	input	[63:0]	slave_io_0_user_interface_datain_12;
	input	[63:0]	slave_io_0_user_interface_datain_13;
	input	[63:0]	slave_io_0_user_interface_datain_14;
	input	[63:0]	slave_io_0_user_interface_datain_15;
	output		slave_io_0_user_interface_write;
	output		slave_io_0_user_interface_read;
	output	[15:0]	slave_io_0_user_interface_chipselect;
	output	[7:0]	slave_io_0_user_interface_byteenable;
endmodule
