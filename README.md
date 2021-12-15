# DE0-nano-SoC Linux Development environment
## -- example device-tree, driver and some tools --

Prerequisite
===============
A bootable Debian system has to be prepared as described in [socfpga_debian](https://github.com/qtplatz/socfpga_debian).

### 1. Synthesys and implemetation

The Intel Quartus project for testing is in the `socfpga_modules/qpf` directory.  You can compile the `soc_system.qpf` project from the `Quartus` IDE or execute the 'make' command under the `socfpga_modules/qpf` directory.

### 2. Edit config.sh

The `cross_kernel_source` variable in the `socfpga_modules/config.sh` file needs to be set appropriately, where it points to Linux kernel source/build directory as relative to your home directory.

### 3. Generate device tree

#### CAUTION: Don't use files under the `socfpga_modules/dts` directory, which are deprecated.  It remains for reference purposes only.  Use `dtsi` directory instead.

The directory `socfpga_modules/dtsi/` contains a files necessary to generate .dts/.dtb file for containing FPGA configuration testing.  The makefile will generate two kinds of .dtb files, where the one for device-tree (DT) overlay via kernel/config interface and the other is for embedded.

DT overly allows you to configure/change the device tree dynamically while the kernel is running.  However, the API for the mainline kernel is not yet natively supported. Intel (or Xilinx) provided kernels are supported DT overlay with behand kernel version.  I would recommend to stay with embedded .dtb file, which can be done by editting soc_system_embedded.dtsi file manually and then compile with kernel source code.  The only advantage of using DT overlay over static (embedded) .dtb is that it does not require rebooting the kernel when changing the device tree.

### 4. Build boot files

The directory 'socfpga_modules/boot` contains a file necessary to generate a set of boot files, including FPGA configuration.  Run `make` will generate necessary files to boot.

| Generated file | Description                |
|----------------|----------------------------|
| boot.scr       | boot script (command list) |
| soc_system.dtb | DT file (binary)           |
| soc_system.dts | DT file (text)             |
| soc_system.rbf | FPGA raw bit file          |

The above files need to be copied to the FAT32 (DOS) partition of the tar
get device.  The command `make install` might be handy not only copy boot files to the device but also install on the target device.

```bash
$ cd ~/src/de0-nano-soc/socfpga_modules/boot
$ make
$ make install
```

```bash
$ ssh <target host name/ip address>
$ make install
```

In case if you get an error `-bash make command not found,` Linux does not have the build commands.  The following command on the target Linux will fix it.

```bash
root@nano~# apt update && apt -y upgrade
root@nano~# apt -y install u-boot-tools libncurses5-dev bc git build-essential cmake dkms
```

### 5. Build Linux kernel modules for FPGA test

```bash
$ SOCFPGA=~/src/de0-nano-soc
$ mkdir -p $SOCFPGA
$ cd $SOCFPGA
$ git clone https://github.com/qtplatz/socfpga_modules
$ cd socfpga_modules
$ cross_target=armhf ./bootstrap.sh
```
Above command-set will create a build directory under `~/build-armhf/socfpga_modules.release/`; continue following commands.

```bash
$ cd ~/src/build-armhf/socfpga_modules.release
$ make -j package
$ cp *.deb <target-ip>:
```

### 6. qtplatz core library for ARM SoC

```bash
$ cd ~/src/qtplatz
$ cross_target=armhf ./bootstran
$ cd ~/src/build-armhf/qtplatz.release
$ make -j4
$ make -j4 package
$ cp *.deb <target-ip>:
```

### Install deb packages and test

Install .deb packaged on the target device; dkms packages first, then other packages.
```bash
nano # dpkg -i <package-name>.deb
```

### Quick test
Following command read 12bit A/D converter on DE0-nano-SoC device, and display values for 999 replicates.
```bash
nano $ adc -c 999
```


