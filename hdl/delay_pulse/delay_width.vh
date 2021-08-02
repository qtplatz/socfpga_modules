/*
 * Delay Generator for ESI-OA-MULTUM-S II
 * Copyright (c) 2019 MS-Cheminformatics LLC
 * Author: Toshinobu Hondo
 */

`ifndef __DELAY_PULSE_T_VH__
 `define __DELAY_PULSE_T_VH__

typedef struct {
   bit [31:0]  delay;
   bit [31:0]  width;
} delay_width_t;

`endif
