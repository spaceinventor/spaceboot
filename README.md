spaceboot
##########

CAN Bootloader for Space Inventor products.

`spaceboot` can upload a software image to either flash slot flash1 or flash0 of a device. Normally a system executes from flash slot0 (unless the boot alternative flag is set). In order to overwrite flash0 `spaceboot` has a built-in bootloader that it can upload to flash slot1 if needed (see the -b flag).

```
Usage: spaceboot [OPTIONS] [COMMANDS] <TARGET>

CAN Bootloader
Copyright (c) 2017 Space Inventor <info@satlab.com>

 Options:

  -i INTERFACE		Use INTERFACE as CAN interface
  -n NODE		Use NODE as own CSP address
  -h 			Show help
  -l 			List embedded images

 Commands (executed in order):

  -r 			Reset to flash0
  -b [filename]		Upload bootloader to flash slot 1
  -f <filename>		Upload file to flash slot 0

 Arguments:

 <target>		CSP node to upload to
```

Examples
--------

`spaceboot -r -b -f obc.bin 7`

This will upload `obc.bin` to node 7 using a bootloader that will temporarily be uploaded to flash slot 1.

```
PING
----
  | bootloader-e70
  | SpaceInventor
  | f66afe6
  | Mar 21 2017 10:25:10
  Switching to flash 0
  20:0  boot_alt             = 0* uint8[1]
  Rebooting..........
  | bootloader-e70
  | SpaceInventor
  | e5055fb
  | Mar 21 2017 15:08:26

BOOTLOADER
----------
  Using embedded image: e70_2
  Upload 88708 bytes to node 0 addr 0x480000
  ................................ - 6 K
  ................................ - 12 K
  ................................ - 18 K
  ................................ - 24 K
  ................................ - 30 K
  ................................ - 36 K
  ................................ - 42 K
  ................................ - 48 K
  ................................ - 54 K
  ................................ - 60 K
  ................................ - 66 K
  ................................ - 72 K
  ................................ - 78 K
  ................................ - 84 K
  ............... - 87 K
  Uploaded 88708 bytes in 7.232 s at 12266 Bps
  ................................ - 6 K
  ................................ - 12 K
  ................................ - 18 K
  ................................ - 24 K
  ................................ - 30 K
  ................................ - 36 K
  ................................ - 42 K
  ................................ - 48 K
  ................................ - 54 K
  ................................ - 60 K
  ................................ - 66 K
  ................................ - 72 K
  ................................ - 78 K
  ................................ - 84 K
  ............... - 87 K
  Downloaded 88708 bytes in 1.903 s at 46614 Bps
  Switching to flash 1
  20:0  boot_alt             = 1* uint8[1]
  Rebooting..........
  | bootloader-e70
  | SpaceInventor
  | f66afe6
  | Mar 21 2017 10:25:10

FLASHING
----------
  Open ../obc/build/obc.bin size 92632
  Upload 92632 bytes to node 0 addr 0x400000
  ................................ - 6 K
  ................................ - 12 K
  ................................ - 18 K
  ................................ - 24 K
  ................................ - 30 K
  ................................ - 36 K
  ................................ - 42 K
  ................................ - 48 K
  ................................ - 54 K
  ................................ - 60 K
  ................................ - 66 K
  ................................ - 72 K
  ................................ - 78 K
  ................................ - 84 K
  ................................ - 90 K
  ... - 90 K
  Uploaded 92632 bytes in 7.459 s at 12418 Bps
  ................................ - 6 K
  ................................ - 12 K
  ................................ - 18 K
  ................................ - 24 K
  ................................ - 30 K
  ................................ - 36 K
  ................................ - 42 K
  ................................ - 48 K
  ................................ - 54 K
  ................................ - 60 K
  ................................ - 66 K
  ................................ - 72 K
  ................................ - 78 K
  ................................ - 84 K
  ................................ - 90 K
  ... - 90 K
  Downloaded 92632 bytes in 1.982 s at 46736 Bps
  Switching to flash 0
  20:0  boot_alt             = 0* uint8[1]
  Rebooting..........

```
