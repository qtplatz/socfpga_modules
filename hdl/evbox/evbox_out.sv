//-------------------------------------------------------------------------------
//-- Title      : LC/GC Event IO Box
//-- Project    : delay/pulse generator for infiTOF
//-- Author     : Toshinobu Hondo, Ph.D.
//-- Copyright (C) 2017 MS-Cheminfomatics LLC
//-------------------------------------------------------------------------------

module evbox_out ( input [N-1:0] data_out
		   , output [N-1:0] io_port  );

   parameter N = 4;

   assign io_port [N-1:0] = data_out [N-1:0];

endmodule


