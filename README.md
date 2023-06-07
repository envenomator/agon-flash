# Agon MOS firmware upgrade utility
The utility version you need, depends on the *current* version of MOS. When you reset your AgonLight, the MOS version is displayed on the first line.

Please follow the guide for your *current* MOS version:
1. [*Current* MOS Version 1.03 or higher](#current-mos-firmware-103-or-higher)
2. [*Current* MOS Version 1.02](#current-mos-firmware-102)
3. [*Current* Version 1.00 or 1.01](#current-mos-firmware-100101-legacy-version)

## Current MOS firmware 1.03 or higher
This version needs at least MOS version 1.03, which supports several enhancements to the MOS file API.
### Installation
1. Make sure to create a 'mos' directory on the microSD card
2. Place the [flash.bin](https://github.com/envenomator/agon-flash/blob/master/binaries/Current%20MOS%20version%201.03%20or%20higher/flash.bin) from in the 'mos' directory
3. Place the required firmware file in the root directory of the microSD card
4. Obtain the CRC32 checksum for the new firmware. The [table below](#valid-mos-crc32-checksums) lists the checksums for common MOS versions. If you like to provide a checksum for your own MOS binary, some good tips are: The crc32 utility on Linux, or a website like https://simplycalc.com/crc32-file.php. For the latter, use the default polynomial of 04C11DB7, upload the firmware and note the result for use in the upgrade utility.

### Target MOS version
Any new MOS version can be flashed using this utility version

## Current MOS firmware 1.02
This version needs at least MOS version 1.02, which supports loadable commands with arguments.
### Installation
1. Make sure to create a 'mos' directory on the microSD card
2. Place the [flash.bin](https://github.com/envenomator/agon-flash/blob/master/binaries/Current%20MOS%20version%201.02/flash.bin) from in the 'mos' directory
3. Place the required firmware file in the root directory of the microSD card
4. Obtain the CRC32 checksum for the new firmware. The [table below](#valid-mos-crc32-checksums) lists the checksums for common MOS versions. If you like to provide a checksum for your own MOS binary, some good tips are: The crc32 utility on Linux, or a website like https://simplycalc.com/crc32-file.php. For the latter, use the default polynomial of 04C11DB7, upload the firmware and note the result for use in the upgrade utility.

### Target MOS version
Any new MOS version can be flashed using this utility version

## Usage for *current* MOS versions 1.02+
```console
Usage: FLASH <filename> <crc32>
```
The provided CRC32 needs to be 4byte, with or without a leading 0x, in hexadecimal format. 

To upgrade from MOS 1.02 to version 1.03 for example:
```console
FLASH firmware103.bin 0x81E397C9
```

## Current MOS firmware 1.00/1.01 (Legacy version)
This version can run on MOS version 1.00 or 1.01 and will do a single upgrade to MOS version 1.02 only.
### Installation
1. Place the [flash_legacy.bin](https://github.com/envenomator/agon-flash/blob/master/binaries/Current%20MOS%20version%20up%20to%201.01/flash_legacy.bin) in the *root* directory of the microSD card
2. Place the [firmware102.bin](https://github.com/envenomator/agon-flash/blob/master/binaries/firmware102.bin) in the root directory of the microSD card

### Target MOS version
Only MOS version 1.02 can be flashed using this utility version. For additional MOS upgrades, you need [this](#current-mos-firmware-102) utility version afterwards.

### Usage for *current* MOS version 1.00/1.01
First make sure you are on the MOS console, presented with the MOS prompt

    *

If you are in BBC Basic (Prompted with '>'), first type *BYE to exit BBC basic and return to MOS.

Load and Jump to the binary in memory:
```console
LOAD flash_legacy.bin
JMP &040000
```

## Valid MOS CRC32 checksums

| MOS version | Filename           | CRC32      |
|-------------|--------------------|------------|
| 1.02        | firmware102.bin    | 0xFE59E98D |
| 1.03        | firmware103.bin    | 0x81E397C9 |

## Workflow
The utility reads in the given firmware file to memory and verifies this against the given CRC32 checksum.
If the file is larger than the amount of flash, the tool will exit.
Upon checksum verification, the user is asked whether or not to proceed.

The utility then creates an image backup of the entire current flash area, stores that in memory and checksums it.
The entire flash is unlocked and erased and the given firmware is programmed.
Verification to the given CRC32 checksum is done on the flash. When this fails, the backup image is reprogrammed to the flash drive in similar fashion and verified to it's original CRC32 checksum.

The MOS currently has no built-in backup code, or second slot, that it can access from flash. If for whatever reason, the checksum of the new firmware should fail, the utility automatically tries to recover by reprogramming the backup code to the flash drive again. If that too fails, the only remaining option is to use a Zilog Programming cable and recover the firmware using it.

## Disclaimer
Having subjected my own gear muuuuultiple times to this utility, I feel it's safe for general use in upgrading the MOS firmware to a new version.
The responsibility for any issues during and/or after the firmware upgrade, lies with the user of this utility.

