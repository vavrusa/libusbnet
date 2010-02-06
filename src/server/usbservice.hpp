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

#ifndef __usbservice_hpp__
#define __usbservice_hpp__
#include "serversocket.hpp"
#include "protocol.hpp"
#include "usbnet.h"
#include <list>

class UsbService : public ServerSocket
{
   public:
   UsbService(int fd = -1);
   ~UsbService();

   /** Reimplemented packet handling.
     */
   virtual bool handle(int fd, Packet& pkt);

   protected:

   /* libusb implementations.
    */

   /* (1) Core functions. */
   void usb_init(int fd, Packet& in);
   void usb_find_busses(int fd, Packet& in);
   void usb_find_devices(int fd, Packet& in);

   /* (2) Device controls. */
   void usb_open(int fd, Packet& in);
   void usb_close(int fd, Packet& in);

   /* (3) Control transfers. */
   void usb_control_msg(int fd, Packet& in);

   private:
   /* libusb data storage */
   std::list<usb_dev_handle*> mOpenList;
};

#endif // __usbservice_hpp__
