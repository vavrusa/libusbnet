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
    \brief libusb prototypes import.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup libusbnet
    @{
  */
#ifndef __usbnet_h__
#define __usbnet_h__

// Include original libusb header
#include <usb.h>

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
