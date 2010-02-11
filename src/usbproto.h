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

#ifndef __libusbproto_h__
#define __libusbproto_h__

#include "protocol.h"

/* Call opcodes.
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
   UsbDetachKernelDriver = CallType  + 10, // int usb_detach_kernel_driver()
   UsbBulkRead           = CallType  + 11, // int usb_bulk_read()
   UsbBulkWrite          = CallType  + 12, // int usb_bulk_write()
   UsbSetConfiguration   = CallType  + 13, // int usb_set_configuration()
   UsbSetAltInterface    = CallType  + 14, // int usb_set_altinterface()
   UsbResetEp            = CallType  + 15, // int usb_resetep()
   UsbClearHalt          = CallType  + 16, // int usb_clear_halt()
   UsbReset              = CallType  + 17, // int usb_reset()
   UsbInterruptRead      = CallType  + 18, // int usb_interrupt_read()
   UsbInterruptWrite     = CallType  + 19  // int usb_interrupt_write()

} Call;

#endif // __libusbproto_h__
