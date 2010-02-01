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
#include "protocol.h"
#include <cstdio>
#include <netinet/tcp.h>
#include <usb.h>

UsbService::UsbService(int fd)
   : ServerSocket(fd)
{
   // Disable TCP buffering
   int flag = 1;
   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

bool UsbService::handle(int fd, int op)
{
   // Packet handling
   switch(op)
   {
      case UsbInit:        usb_init(fd, 0, NULL);         break;
      case UsbFindBusses:  usb_find_busses(fd, 0, NULL);  break;
      case UsbFindDevices: usb_find_devices(fd, 0, NULL); break;
      default:
         fprintf(stderr, "Call:  0x%x unhandled call type (fd %d).\n", op, fd);
         return false;
         break;
   }

   return true;
}

void UsbService::usb_init(int fd, int size, const char* data)
{
   // Call, no ACK
   printf("Call: usb_init()\n");
   ::usb_init();
}

void UsbService::usb_find_busses(int fd, int size, const char* data)
{
   // Call
   int res = ::usb_find_busses();
   printf("Call: usb_find_busses() = %d\n", res);

   // Send result
   char buf[32];
   packet_t pkt = pkt_create(buf, 32);
   pkt_init(&pkt, UsbFindBusses);
   pkt_append(&pkt, IntegerType, sizeof(res), &res);
   pkt_send(fd, &pkt);
}

void UsbService::usb_find_devices(int fd, int size, const char* data)
{
   // Call
   int res = ::usb_find_devices();
   printf("Call: usb_find_devices() = %d\n", res);

   // Send result
   char buf[32];
   packet_t pkt = pkt_create(buf, 32);
   pkt_init(&pkt, UsbFindDevices);
   pkt_append(&pkt, IntegerType, sizeof(res), &res);
   pkt_send(fd, &pkt);
}
