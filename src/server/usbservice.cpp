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
#include <netinet/tcp.h>
#include <cstdio>

UsbService::UsbService(int fd)
   : ServerSocket(fd)
{
   // Disable TCP buffering
   int flag = 1;
   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

UsbService::~UsbService()
{
   // Close open devices
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      printf("UsbService: closing open device %p.", *i);
      ::usb_close(*i);
   }
   mOpenList.clear();
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
      case UsbOpen:        usb_open(fd, pkt);         break;
      case UsbClose:       usb_close(fd, pkt);        break;
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

void UsbService::usb_open(int fd, Packet& in)
{
   Symbol sym(in);
   unsigned busid = sym.asUInt();
   sym.next();
   unsigned devid = sym.asUInt();

   // Find device
   struct usb_device* rdev = NULL;
   for(struct usb_bus* bus = ::usb_get_busses(); bus; bus = bus->next) {

      // Find bus
      if(bus->location == busid) {
         for(struct usb_device* dev = bus->devices; dev; dev = dev->next) {

            // Find device
            if(dev->devnum == devid) {
               rdev = dev;
               break;
            }
         }
         break;
      }
   }

   // Open device
   usb_dev_handle* udev = NULL;
   if(rdev != NULL) {
      udev = ::usb_open(rdev);
      mOpenList.push_back(udev);
   }

   // Return result
   Packet pkt(UsbOpen);
   pkt.addUInt8(udev != NULL);
   pkt.send(fd);

   printf("Call: usb_open(%u:%u) = %u\n", busid, devid, udev != NULL);
}

void UsbService::usb_close(int fd, Packet& in)
{
   Symbol sym(in);
   unsigned busid = sym.asUInt();
   sym.next();
   unsigned devid = sym.asUInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->bus->location == busid && h->device->devnum == devid) {
         res = ::usb_close(h);
         mOpenList.erase(i);
         break;
      }
   }

   // Return result
   Packet pkt(UsbClose);
   pkt.addUInt8(res == 0);
   pkt.send(fd);

   printf("Call: usb_close(%u:%u) = %u\n", busid, devid, res == 0);
}
