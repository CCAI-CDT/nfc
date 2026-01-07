// Modifications by Dan Jackson, 2026
// File partially based on: https://github.com/nfc-tools/libnfc/blob/master/libnfc/drivers/acr122_usb.c#L301

// Requires, on macOS:
//   brew install libusb

// To compile, on macOS:
//   gcc -I$(brew --prefix)/include -L$(brew --prefix)/lib usb-list.c -lusb -o usb-list && ./usb-list

// To list USB devices -- alternatively: ioreg -p IOUSB -l -w0
//   ./usb-list
// ...outputs UID of devices, one per line.

// For usb.h on macOS with Homebrew, see:  /opt/homebrew/include/usb.h

// usb:072f:2200 -- location: usb:###:###

#include <stdlib.h>
#include <stdio.h>


//#include "buses/usbbus.h"
#ifndef _WIN32
// Under POSIX system, we use libusb (>= 0.1.12)
#include <stdint.h>
#include <usb.h>
#define USB_TIMEDOUT ETIMEDOUT
#define _usb_strerror( X ) strerror(-X)
#else
// Under Windows we use libusb-win32 (>= 1.2.5)
#include <lusb0_usb.h>
#define USB_TIMEDOUT 116
#define _usb_strerror( X ) usb_strerror()
#endif

#include <stdbool.h>
#include <string.h>

int usb_prepare(void)
{
  static bool usb_initialized = false;
  if (!usb_initialized) {
    usb_init();
    usb_initialized = true;
  }

  int res;
  // usb_find_busses will find all of the busses on the system. Returns the
  // number of changes since previous call to this function (total of new
  // busses and busses removed).
  if ((res = usb_find_busses()) < 0) {
    //log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to find USB busses (%s)", _usb_strerror(res));
    return -1;
  }
  // usb_find_devices will find all of the devices on each bus. This should be
  // called after usb_find_busses. Returns the number of changes since the
  // previous call to this function (total of new device and devices removed).
  if ((res = usb_find_devices()) < 0) {
    //log_put(LOG_GROUP, LOG_CATEGORY, NFC_LOG_PRIORITY_ERROR, "Unable to find USB devices (%s)", _usb_strerror(res));
    return -1;
  }
  return 0;
}


static size_t usb_scan()
{
  usb_prepare();
  size_t device_found = 0;
  uint32_t uiBusIndex = 0;
  struct usb_bus *bus;
  for (bus = usb_get_busses(); bus; bus = bus->next) {
    struct usb_device *dev;
    for (dev = bus->devices; dev; dev = dev->next, uiBusIndex++) {
      // dev->descriptor.idVendor
      // dev->descriptor.idProduct

      if (dev->config == NULL || dev->config->interface == NULL || dev->config->interface->altsetting == NULL) continue;
      //if (dev->config->interface->altsetting->bNumEndpoints < 2) continue;

      usb_dev_handle *udev = usb_open(dev);
      if (udev == NULL) continue;

      char manufacturer[256] = "", product[256] = "", serial[256] = "";
      if (dev->descriptor.iManufacturer) usb_get_string_simple(udev, dev->descriptor.iManufacturer, manufacturer, sizeof(manufacturer));
      if (dev->descriptor.iProduct) usb_get_string_simple(udev, dev->descriptor.iProduct, product, sizeof(product));
      if (dev->descriptor.iSerialNumber) usb_get_string_simple(udev, dev->descriptor.iSerialNumber, serial, sizeof(serial));

      // bus->dirname and dev->filename are three-digit zero-padded numbers equivalent to:
      //   uint32_t location = bus->location;
      //   uint8_t devnum = dev->devnum;

      // The port_numbers path would be better, but not available in this version of libusb.
      /*
      uint8_t bus_number = libusb_get_bus_number(udev);
      printf("  Bus Number: %u\n", bus_number);
      uint8_t port_number = libusb_get_port_number(udev);
      printf("  Port Number: %u\n", port_number);
      uint8_t device_address = libusb_get_device_address(udev);
      printf("  Device Address: %u\n", device_address);
      uint8_t port_numbers[16];
      int num_port_numbers = libusb_get_port_numbers(udev, port_numbers, sizeof(port_numbers));
      printf("  Port Numbers: ");
      for (int i = 0; i < num_port_numbers; i++) {
        if (i > 0) printf(".");
        printf("%u", port_numbers[i]);
      }
      printf("\n");
      */

      printf("%s:%s\t%04x:%04x\t%s\t%s\t%s\n", bus->dirname, dev->filename, dev->descriptor.idVendor, dev->descriptor.idProduct, manufacturer, product, serial);

      usb_close(udev);

      device_found++;
    }
  }
  return device_found;
}

int main(int argc, char *argv[])
{
    return usb_scan();
}
