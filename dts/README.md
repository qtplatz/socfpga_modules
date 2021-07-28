
This directory is for reference purposes only.  Don't appply .dts/.dtb for boot filesystem.
=========

Until kernel 4.4, the .dts and .dtb files generated using sopc2dts.jar under "this" directory work fine.
However, the .dts file no longer booted Linux properly after kernel 4.10 (or around) and beyond.

The "socfpga_modules" project is configured with 5.10 kernel, which needs to configure the .dts file manually on the <socfpga_modules>/boot directory. Still, contents produced by sopc2dts under this directory are helpful enough to configure the .dts file manually.
