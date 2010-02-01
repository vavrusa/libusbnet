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

class UsbService : public ServerSocket
{
   public:
   UsbService(int fd = -1);

   /** Reimplemented packet handling.
     */
   virtual bool handle(int fd, int op);

   protected:

   /* libusb implementations. */
   void usb_init(int fd, int size, const char* data);
   void usb_find_busses(int fd, int size, const char* data);
};

#endif // __usbservice_hpp__
