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

#include "usbservice.hpp"
#include "protocol.hpp"
#include <netinet/tcp.h>
#include <usb.h>
#include <cstdio>

UsbService::UsbService(int fd)
   : ServerSocket(fd)
{
   // Disable TCP buffering
   int flag = 1;
   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

bool UsbService::handle(int fd, Packet& pkt)
{
   // Check size
   if(pkt.size() <= 0)
      return false;

   // Packet handling
   switch(pkt.op())
   {
      case UsbInit:        usb_init(fd, pkt);         break;
      case UsbFindBusses:  usb_find_busses(fd, pkt);  break;
      case UsbFindDevices: usb_find_devices(fd, pkt); break;
      default:
         fprintf(stderr, "Call:  0x%02x unhandled call type (fd %d).\n", pkt.op(), fd);
         return false;
         break;
   }

   return true;
}

void UsbService::usb_init(int fd, Packet& in)
{
   // Call, no ACK
   printf("Call: usb_init()\n");
   ::usb_init();
}

void UsbService::usb_find_busses(int fd, Packet& in)
{
   // Call
   // Can't guarantee correct number in case of multi-client environment
   int res = ::usb_find_busses();
   printf("Call: usb_find_busses() = %d\n", res);

   // Send result
   Packet pkt(UsbFindBusses);
   pkt.addUInt32(res);
   pkt.send(fd);
}

void UsbService::usb_find_devices(int fd, Packet& in)
{
   // WARNING: Can't guarantee correct number in case of multi-client environment
   int res = ::usb_find_devices();
   printf("Call: usb_find_devices() = %d\n", res);

   // Prepare result packet
   Packet pkt(UsbFindDevices);
   pkt.addUInt32(res);

   // Add existing busses and devices
   struct usb_bus* bus = 0;
   for(bus = ::usb_get_busses(); bus; bus = bus->next) {

      /* char dirname[PATH_MAX + 1];
         struct usb_device *devices;
         u_int32_t location;
         struct usb_device *root_dev;
       */

      Block block = pkt.writeBlock(StructureType);
      block.addString(bus->dirname);
      block.addUInt32(bus->location);

      // TODO: only top-level devices supported
      for(struct usb_device* dev = bus->devices; dev; dev = dev->next) {

         /* char filename[PATH_MAX + 1];
            struct usb_device_descriptor descriptor;
            u_int8_t devnum;
         */
         Block devBlock = block.writeBlock(SequenceType);
         devBlock.addString(dev->filename);
         devBlock.addData((const char*) &(dev->descriptor), sizeof(dev->descriptor));
         devBlock.addUInt8(dev->devnum);
         devBlock.finalize();
      }

      // Finalize block
      block.finalize();
   }

   // Send result
   pkt.send(fd);
}
