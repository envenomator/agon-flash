## Title:       send.py
## Author:      Jeroen Venema
## Created:     25/10/2022
## Last update: 11/09/2023

## syntax
## send.py FILENAME <PORT> <BAUDRATE>
## 

## Modinfo:
## 25/10/2022 initial version
## 10/09/2023 Script converts binary file to Intel Hex during transmission. 
##            Using defaults as constants.
## 11/09/2023 Wait time variable introduced for handling PC serial drivers with low buffer memory

DEFAULT_START_ADDRESS = 0x40000
DEFAULT_SERIAL_PORT   = 'COM11'
DEFAULT_BAUDRATE      = 115200
DEFAULT_LINE_WAITTIME = 0       ## A value of +/- 0.003 Helps PC serial drivers with low buffer memory

def errorexit(message):
  print(message)
  print('Press ENTER to continue')
  input()
  exit()
  return

import sys
import time
import os.path
import tempfile
import serial.tools.list_ports
try:
  import serial
except ModuleNotFoundError:
  errorexit('Please install the \'pyserial\' module with pip')
try:
  from intelhex import IntelHex
except ModuleNotFoundError:
  errorexit('Please install the \'intelhex\' module with pip')



if len(sys.argv) == 1 or len(sys.argv) >4:
  sys.exit('Usage: send.py FILENAME <PORT> <BAUDRATE>')

if not os.path.isfile(sys.argv[1]):
  sys.exit(f'Error: file \'{sys.argv[1]}\' not found')

if len(sys.argv) == 2:
  #serialport = DEFAULT_SERIAL_PORT
  serialports = serial.tools.list_ports.comports()
  if len(serialports) > 1:
    sys.exit("Multiple COM ports - cannot automatically select");
  serialport = str(serialports[0]).split(" ")[0]
if len(sys.argv) >= 3:
  serialport = sys.argv[2]

if len(sys.argv) == 4:
  baudrate = int(sys.argv[3])
else:
  baudrate = DEFAULT_BAUDRATE

nativehexfile = ((sys.argv[1])[-3:] == 'hex') or ((sys.argv[1])[-4:] == 'ihex')

# report parameters used
print(f'Sending \'{sys.argv[1]}\' ', end="")
if nativehexfile: print('as native hex file')
else: 
  print('in Intel Hex format')
  print(f'Using start address 0x{DEFAULT_START_ADDRESS:x}')
print(f'Using serial port {serialport}')
print(f'Using Baudrate {baudrate}')

if nativehexfile:
  file = open(sys.argv[1], "r")
  content = file.readlines()
else:
  # Instantiate ihex object and load binary file to it, write out as ihex format to temp file
  ihex = IntelHex()
  file = tempfile.TemporaryFile("w+t")
  ihex.loadbin(sys.argv[1], offset=DEFAULT_START_ADDRESS)
  ihex.write_hex_file(file)
  file.seek(0)

ser = serial.Serial()
ser.baudrate = baudrate
ser.port = serialport
ser.setDTR(False)
ser.setRTS(False)
try:
    ser.open()
    print('Opening serial port...')
    time.sleep(1)
    print('Sending data...')

    if nativehexfile:
      for line in content:
        ser.write(str(line).encode('ascii'))
        time.sleep(DEFAULT_LINE_WAITTIME)
    else:
      for line in file:
        ser.write(str(line).encode('ascii'))
        time.sleep(DEFAULT_LINE_WAITTIME)

    #time.sleep(1)
    ser.close()
except serial.SerialException:
    errorexit('Error: serial port unavailable')

file.close()
