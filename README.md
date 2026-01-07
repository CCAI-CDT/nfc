<!-- spell-checker:words PICC MIFARE FeliCa Innovision ioreg IOUSB IOCF DEXT kext driverkit libnfc lnfc -->

# NFC Reader: ACR122U

<!--

To do list:
* Python wrapper to identify all readers and spawn nfc-poll for each reader, collect output and provide a unified interface
* Attempt to list and match on USB serial number, or USB location ID where serial number is missing or 0.

-->

## Setup

```bash
brew install libnfc
gcc -I$(brew --prefix)/include -L$(brew --prefix)/lib nfc-poll.c -o nfc-poll -lnfc
```

## Information

AC122U:

* https://www.acs.com.hk/acr122.php
* https://www.acs.com.hk/en/products/3/acr122u-usb-nfc-reader/

libNFC:

* https://github.com/nfc-tools/libnfc
* http://nfc-tools.org/ -- https://web.archive.org/web/20210815064122/http://nfc-tools.org/index.php?title=Libnfc#Mac_OS_X

For macOS, see:

* https://formulae.brew.sh/formula/libnfc
* https://stackoverflow.com/questions/29998196/acr122u-sdk-for-mac-osx


## Misc

List USB devices on macOS:

```bash
ioreg -p IOUSB -l -w0
```

```
ACR122U PICC Interface@40132000  <class IOUSBHostDevice, id 0x10413d2a9, registered, matched, active, busy 0 (24 ms), retain 26>
{
  "sessionID" = 33498739586070
  "USBSpeed" = 1
  "idProduct" = 8704
  "iManufacturer" = 1
  "bDeviceClass" = 0
  "IOPowerManagement" = {"PowerOverrideOn"=Yes,"DevicePowerState"=2,"CurrentPowerState"=2,"CapabilityFlags"=32768,"MaxPowerState"=2,"DriverPowerState"=0}
  "bcdDevice" = 532
  "bMaxPacketSize0" = 8
  "iProduct" = 2
  "iSerialNumber" = 0
  "bNumConfigurations" = 1
  "kUSBContainerID" = "cd9b1153-9034-467e-99da-d8aafa9aa0b0"
  "UsbDeviceSignature" = <2f07002214020000000b0000>
  "locationID" = 1074995200
  "bDeviceSubClass" = 0
  "bcdUSB" = 272
  "USB Product Name" = "ACR122U PICC Interface"
  "USB Address" = 9
  "IOCFPlugInTypes" = {"9dc7b780-9ec0-11d4-a54f-000a27052861"="IOUSBHostFamily.kext/Contents/PlugIns/IOUSBLib.bundle"}
  "kUSBCurrentConfiguration" = 1
  "bDeviceProtocol" = 0
  "USBPortType" = 1
  "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  "USB Vendor Name" = "ACS"
  "Device Speed" = 1
  "idVendor" = 1839
  "kUSBProductString" = "ACR122U PICC Interface"
  "IOGeneralInterest" = "IOCommand is not serializable"
  "kUSBAddress" = 9
  "kUSBVendorString" = "ACS"
}
```


## Commands

## ATR Generation

If the reader detects a PICC, an ATR will be sent to the PC/SC driver for identifying the PICC.

e.g. ATR for MIFARE 1K:

```
3Bh  # Initial header
8Fh  # T0 (higher nibble 8 means no TA1, TB1, TC1 - only TD1 follows)
80h  # TD1
01h  # TD2
80h  # T1 
4Fh  # Application identifier presence indicator
0Ch  # Length=12
xx xx xx xx xx  # RID (PC/SC workgroup)
03h  # Standard (03=ISO 14443A, Part 3)
00 01h  # Card name (00 01=MIFARE Classic 1K, 00 02=MIFARE Classic 4K, 00 03=MIFARE Ultralight, ...)
00 00 00 00h  # RFU
6Ah  # XOR of of bytes T0 to Tk (before RFU)
```




## Get data

To get the serial number of the connected PICC.

```
Send: FFh, CAh, 00h, 00h, 04h
Receive: (LSB) ... (MSB) SW1 SW2
```

To get the ATS of the connected ISO 14443 A PICC.

```
FFh, CAh, 01h, 00h, 04h
Receive: (ATS) SW1 SW2
```

Response Codes:

```
Success SW1=90h SW2=00h The operation completed successfully.
Error SW1=63h SW2=00h The operation failed.
Error SW1=6Ah SW2=81h Function not supported.
```

### Read 8-byte random number

`ACR122U PICC Interface`, `VID=072F` & `PID=2200`.

Read 8 bytes random number from an ISO 14443-4 Type A PICC (DESFire):

```
Send: 0A 00
Receive: AF (AF=status code) ## ## ## ## ## ## ## ##
```


### Get Status

Get the current setting of the contactless interface

```
<< FF 00 00 00 02 D4 04h
>> D5 05h [Err] [Field] [NbTg] [Tg] [BrRx] [BrTx] [Type] 80 90 00h
Or if no tag is in the field
>> D5 05 00 00 00 80 90 00h
[Err] is an error code corresponding to the latest error detected.
Field indicates if an external RF field is present and detected (Field = 01h) or not (Field = 00h).
[NbTg] is the number of targets. The default value is 1.
[Tg]: logical number
[BrRx] : bit rate in reception
00h: 106 Kbps
01h: 212 Kbps
02h: 424 Kbps
[BrTx] : bit rate in transmission
00h: 106 Kbps
01h: 212 Kbps
02h: 424 Kbps
[Type ]: modulation type
00h: ISO 14443 or MIFARE
10h: FeliCa
01h: Active mode
02h: Innovision Jewel tag
```
