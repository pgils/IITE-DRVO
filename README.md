# IITE-DRVO
A simple character device driver for reading a Bosch BMP180 sensor module

### References
- http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device
- https://github.com/akoskovacs/kmod_example
- https://elinux.org/Interfacing_with_I2C_Devices#Beagleboard_I2C2_Enable
- https://beagleboard.org/static/BBxMSRM_latest.pdf
- https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf
- https://www.howtoforge.com/reading-files-from-the-linux-kernel-space-module-driver-fedora-14

# Testing the driver
## I2C using the bmp280 driver
The IIO bmp280 driver in mainline linux exposes the sensor readouts to sysfs.

### Hardware connection
The main expansion header on the BB-xM exposes the I2C2 interface. The others (total 4) are used by various connectors; HDMI, Camera, etc.
|Pin#|Description|BMP180 pin|
|----|-----------|----------|
|1   |VIO_1V8    |VIN       |
|23  |I2C2_SDA   |SDA       |
|24  |I2C2_SCL   |SCL       |
|28  |GND        |GND       |

### Detection
The I2C2 interface shows up as /dev/i2c-1. This bus can be probed for devices with `i2cdetect`:
```
$ i2cdetect -r 1

     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- 77
```

The BMP180's slave address is **"0xEF (read) and 0xEE (write)"** according to [it's datasheet][ds_bmp180]. Without the R/W bit that leaves **0x77**.

Tell the kernel the device at this address is a BMP180 sensor:
```
$ echo "bmp180 0x77" | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
```
The drivers will be loaded automagically:
```
$ lsmod|grep bmp
bmp280_i2c             16384  0
bmp280                 20480  1 bmp280_i2c
```

Read the data:
```
$ cat /sys/bus/i2c/devices/1-0077/iio\:device1/in_*_input
101.463000000
23700
```

[ds_bmp180]: https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

## Using the drvoscd module
This requires the bmp280 driver is loaded and `/sys/bus/i2c/devices/1-0077/iio\:device1/in_*_input` is available.
```
$ git clone https://github.com/pgils/IITE-DRVO.git
$ cd IITE-DRVO
$ make insert
$ echo "pressure"|sudo tee /dev/drvoscd
$ sudo cat /dev/drvoscd
101.792000000
```

# Integrating into IITE-LINUX-xM
Compile the `drvoscd` driver for [IITE-LINUX-xM][repo_lxm]

[repo_lxm]: https://github.com/pgils/IITE-LINUX-xM:
```
$ cd linux-stable
$ git apply ../IITE-DRVO/patches/0001-dts-omap-beagle-xm-map-bmp180-to-I2C2.patch
$ make ARCH=${CLFS_ARCH} CROSS_COMPILE=${CLFS_TARGET}- \
    INSTALL_MOD_PATH=${CLFS}/targetfs modules_install
```
```
$ git clone https://github.com/pgils/IITE-DRVO.git
$ cd IITE-DRVO
$ make ARCH=${CLFS_ARCH} CROSS_COMPILE=${CLFS_TARGET}- \
        INSTALL_MOD_PATH=${CLFS_TARGET_FS} \
        -C ${CLFS_TARGET_FS}/lib/modules/5.6.12/build \
        M=$PWD modules_install
```
```
$ cp arch/arm/boot/{zImage,dts/omap3-beagle-xm.dtb} ${CLFS_TARGET_FS}/boot/
```
