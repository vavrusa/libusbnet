/***************************************************************************
*   Copyright (C) 2009 Marek Vavrusa <marek@vavrusa.com>                  *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Library General Public License as       *
*   published by the Free Software Foundation; either version 2 of the    *
*   License, or (at your option) any later version.                       *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with this program; if not, write to the                 *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/
/*! \file usbnet.h
    \brief Protocol implementation for libusb.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup libusbnet
    @{
  */
#ifndef __usbnet_h__
#define __usbnet_h__

// Include original libusb header
#include <usb.h>
#include "protobase.h"

/** libusb opcode definition.
 */
typedef enum {
   NullRequest           = CallType,       // ping
   UsbInit               = CallType  +  1, // usb_init()
   UsbFindBusses         = CallType  +  2, // usb_find_busses()
   UsbFindDevices        = CallType  +  3, // usb_find_devices()
   UsbGetBusses          = CallType  +  4, // usb_bus* usb_get_busses()
   UsbOpen               = CallType  +  5, // usb_dev_handle *usb_open()
   UsbClose              = CallType  +  6, // int usb_close()
   UsbControlMsg         = CallType  +  7, // int usb_control_msg()
   UsbClaimInterface     = CallType  +  8, // int usb_claim_interface()
   UsbReleaseInterface   = CallType  +  9, // int usb_release_interface()
   UsbGetKernelDriver    = CallType  + 10, // int usb_get_kernel_driver()
   UsbDetachKernelDriver = CallType  + 11, // int usb_detach_kernel_driver()
   UsbBulkRead           = CallType  + 12, // int usb_bulk_read()
   UsbBulkWrite          = CallType  + 13, // int usb_bulk_write()
   UsbSetConfiguration   = CallType  + 14, // int usb_set_configuration()
   UsbSetAltInterface    = CallType  + 15, // int usb_set_altinterface()
   UsbResetEp            = CallType  + 16, // int usb_resetep()
   UsbClearHalt          = CallType  + 17, // int usb_clear_halt()
   UsbReset              = CallType  + 18, // int usb_reset()
   UsbInterruptRead      = CallType  + 19, // int usb_interrupt_read()
   UsbInterruptWrite     = CallType  + 20  // int usb_interrupt_write()

} Call;

/** \private
    @from: libusb/usbi.h:41
    \warning Matches libusb-0.1.12, may loss binary compatibility.
 */
struct usb_dev_handle {
   int fd;

   struct usb_bus *bus;
   struct usb_device *device;

   int config;
   int interface;
   int altsetting;

   /* Added by RMT so implementations can store other per-open-device data */
   void *impl_info;
};

#endif // __usbnet_h__
/** @} */
