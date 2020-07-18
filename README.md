STM32MP157C RPMSG-netlink test tool
----

[![dimtass](https://circleci.com/gh/dimtass/stm32mp1-cmake-rpmsg-test.svg?style=svg)](https://circleci.com/gh/dimtass/stm32mp1-cmake-rpmsg-test)

This code is based on the cmake template for STM32MP157C which is located [here](https://github.com/dimtass/stm32mp1-cmake-template).

This repo contains the source code of the firwmare for the CM4 MPU on the STM32MP1
and a Linux tool for the CA CPU. Both are using OpenAMP to transfer data between
the MPU and CPU via the virtual UART/TTY.

> Note: There is a blog post here which explains how to use this test [here](https://www.stupid-projects.com/benchmarking-the-stm32mp1-ipc-between-the-mcu-and-cpu-part-2/).

## Build the CM firmware
To build the firmware you need to clone the repo in any directory and then inside
that directrory run the command:

```sh
SRC=src_hal ./build.sh
```

The above command assumes that you have a toolchain in your `/opt` folder. In case,
you want to point to a specific toolchain path, then run:

```sh
TOOLCHAIN_DIR=/path/to/toolchain SRC=src_hal ./build.sh
```

Or you can edit the `build.sh` script and add your toolchain path.

It's better to use Docker to build the image. To do that run this command:
```sh
docker run --rm -it -v $(pwd):/tmp -w=/tmp dimtass/stm32-cde-image:latest -c "SRC=src_hal ./build.sh"
```

In order to remove any previous builds, then run:
```sh
docker run --rm -it -v $(pwd):/tmp -w=/tmp dimtass/stm32-cde-image:latest -c "CLEANBUILD=true SRC=src_hal ./build.sh"
```

## Build the CA tool
The CA tool is located in the `CA7-source` folder and it's a cmake project. You can build it
on you x86_64 host, but that doesn't make much sense. Therefore, you need to cross-compile
the tool. I'm using a Yocto SDK that I built my self for testing, but there's also a recipe
[here]() that you can use bitbake to built it. In case you use the Yocto SDK, then you need
to source the SDK environment and then build like this:

```sh
cd CA7-source
mkdir build-armhf
cd build-armhf
source /opt/st/stm32mp1-discotest/3.1-snapshot/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi 
cmake ..
make
```

Then you need to copy the executable to your `/home/root` folder and run it like this:
```sh
./tty-test-client /dev/ttyRPMSG0
```

This is a sample of the output:

```
- 17:56:22.460 INFO: Application started
- 17:56:22.461 INFO: Initialized netlink client.
- 17:56:22.468 INFO: Initialized buffer with CRC16: 0x1818
- 17:56:22.469 INFO: ---- Creating tests ----
- 17:56:22.469 INFO: -> Add test: size=512
- 17:56:22.469 INFO: -> Add test: size=1024
- 17:56:22.469 INFO: -> Add test: size=2048
- 17:56:22.469 INFO: -> Add test: size=4096
- 17:56:22.469 INFO: -> Add test: size=8192
- 17:56:22.469 INFO: -> Add test: size=16384
- 17:56:22.469 INFO: -> Add test: size=32768
- 17:56:22.469 INFO: ---- Starting tests ----
- 17:56:22.494 INFO: -> b: 512, nsec: 24818304, bytes sent: 20
- 17:56:22.522 INFO: -> b: 1024, nsec: 27429189, bytes sent: 20
- 17:56:22.551 INFO: -> b: 2048, nsec: 28360484, bytes sent: 20
- 17:56:22.589 INFO: -> b: 4096, nsec: 36787683, bytes sent: 20
- 17:56:22.642 INFO: -> b: 8192, nsec: 51152197, bytes sent: 20
- 17:56:22.734 INFO: -> b: 16384, nsec: 87905755, bytes sent: 20
- 17:56:22.904 INFO: -> b: 32768, nsec: 162339165, bytes sent: 20
```

In the last rows `b` is the block size, `n` is the number of blocks and `nsec` is the number
of nsecs that the transfer lasted. The timer used for the benchmark is running on the Linux
side, so there it might not be very accurate, but that doesn't really matter as the time the
transactions take are in the range of msecs.

> Note: For some reason when sending more than 5KB the virtual TTY in the Linux side hangs.
It doesn't matter if the block size is small or large, as long a single transaction sends more
than 5KB the issue occures.

## Loading the firmware to CM4
To load the firmware on the Cortex-M4 MCU you need to scp the firmware `.elf` file in the
`/lib/firmware` folder of the Linux instance of the STM32MP1. Then you also need to copy the
`fw_cortex_m4.sh` script on the `/home/root` (or anywhere you like) and then run this command
as root.
```sh
./fw_cortex_m4.sh start
```

To stop the firmware run:
```sh
./fw_cortex_m4.sh stop
```

> Note: The console of the STM32MP1 is routed in the micro-USB connector `STLINK CN11` which
in case of my Ubuntu shows up as `/dev/ttyACMx`.

When you copy the `./fw_cortex_m4_netlink.sh` you need also to enable the execution flag with:
```sh
chmod +x fw_cortex_m4.sh
```

If the firmware is loaded without problem you should see an output like this:
```sh
[ 4090.716351] remoteproc remoteproc0: powering up m4
[ 4090.721409] remoteproc remoteproc0: Booting fw image stm32mp157c-rpmsg-netlink.elf, size 696716
[ 4090.729447]  mlahb:m4@10000000#vdev0buffer: assigned reserved memory node vdev0buffer@10042000
[ 4090.739089] virtio_rpmsg_bus virtio0: rpmsg host is online
[ 4090.747130]  mlahb:m4@10000000#vdev0buffer: registered virtio0 (type 7)
[ 4090.749936] virtio_rpmsg_bus virtio0: creating channel rpmsg-netlink addr 0x0
[ 4090.752312] remoteproc remoteproc0: remote processor m4 is now up
 ```


## Loading the kernel module
Before run the CA7 tool and load the firmware on the CM4 you need to load the kernel module.
To do this run this command inside the `/home/root` folder.

> Note: If you've built the image using [this](https://github.com/dimtass/meta-stm32mp1-bsp-base)
Yocto BSP base layer then the module is already loaded.

```sh
insmod rpmsg_netlink.ko
```

When you run the above command you'll see in `dmesg` this:
```
[ 4170.279590] rpmsg_netlink virtio0.rpmsg-netlink.-1.0: rpmsg-netlink created netlink socket
[ 4170.286882] rpmsg_netlink_drv_init(rpmsg_sdb): Init done
```

To unload the module run:
```sh
rmmod rpmsg_netlink
```

When you run the above command you'll see in `dmesg` this:
```
[ 4240.350540] rpmsg_netlink_drv_exit(rpmsg_sdb): Exit
```

## Testing the firmware
To test the firmware run those commands inside the `/home/root` folder

```sh
insmod rpmsg_netlink.ko
./fw_cortex_m4.sh start
./rpmsg-netlink-client
```

## Debug serial port
The firmware also supports a debug UART on the CM4. This port is mapped to UART7 and the
Arduino connector pins. The pinmap is the following:

pin | Function
-|-
D0 | Rx
D1 | Tx

You can connect a USB-to-UART module to those pins and the GND and then open the tty port
on your host. The port supports 115200 baudrate. When the firmware loads on the CM4 then
you should see this messages:

```sh
[00000.009][INFO ]Cortex-M4 boot successful with STM32Cube FW version: v1.2.0
[00000.016][INFO ]rpmsg-netlink started
```

## Using the cmake template in Yocto
TBD

## License
Just MIT.

## Author
Dimitris Tassopoulos <dimtass@gmail.com>