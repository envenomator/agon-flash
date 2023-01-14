# Agon MOS firmware upgrade utility
This utility comes in two versions:
1. A generic version that will flash new versions of Agon MOS, needing at least baseline version 1.02. This version is loaded as a MOS command and can take arguments from the commandline.
2. A 'legacy' version that can upgrade MOS 1.00 or 1.01 to MOS 1.02 only. Afterwards, the generic version can be used to upgrade to later MOS versions.

## Generic version (Needs current MOS firmware 1.02)
This version needs at least MOS version 1.02, which accepts loadable commands with arguments
### Installation
1. Make sure to create a 'mos' directory on the microSD card
2. Place the flash.bin in the 'mos' directory
3. Place the required firmware file in the root directory of the microSD card
4. Obtain the CRC32 checksum for the new firmware. On Linux you can use the crc32 utility. A suitable website to obtain this might be https://simplycalc.com/crc32-file.php. Use the default polynomial of 04C11DB7, upload the firmware and note the result for use in the utility.

### Usage
Run the upgrade utility as - FLASH \<filename\> \<crc32\>. The CRC32 needs to be 4byte, with or without a leading 0x. 

I.e. to upgrade to example firmware version 1.2, with checksum 0xe6834aa8 can be performed with:
```console
FLASH firmware12.bin 0xe6834aa8
```
## Legacy version (MOS firmware 1.00/1.01)
This version can run on MOS version 1.00 or 1.01 and will do a single upgrade to MOS version 1.02
### Installation
1. Place the flash_legacy.bin in the root directory of the microSD card
2. Place the firmware102.bin in the root directory of the microSD card

### Usage
Load and Jump to the binary in memory:
```console
LOAD flash_legacy.bin
JMP &040000
```
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

