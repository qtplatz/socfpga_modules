#!/bin/bash
# Recompile with:
# mkimage -C none -A arm -T script -d boot.cmd boot.scr

setenv fsck.repair yes
setenv fbcon map:0
setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk0p2 rootfstype=ext4 rw rootwait fsck.repair=${fsck.repair} panic=10 ${extra} fbcon=${fbcon}

setenv fdt_ovaddr_r 02040000
setenv rbf_addr_r   02100000
setenv rbf_size       200000

# load fpga bitstream
fatload mmc 0:1 ${rbf_addr_r} soc_system.rbf
fpga load 0 ${rbf_addr_r} ${rbf_size}
bridge enable

fatload mmc 0 ${kernel_addr_r} zImage
fatload mmc 0 ${fdt_addr_r} u-boot.dtb

# fatload mmc 0 ${fdt_ovaddr_r}  overlay.dtbo
# fdtaddr ${fdt_addr_r}
# fdt resize 8192
# fdt apply ${fdt_ovaddr_r}

bootz ${kernel_addr_r} - ${fdt_addr_r}
