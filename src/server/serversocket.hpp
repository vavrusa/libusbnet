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
/*! \file serversocket.hpp
    \brief Abstract server socket and packet handler.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup server
    @{
  */
#pragma once
#ifndef __serversocket_hpp__
#define __serversocket_hpp__
#include "socket.hpp"
#include "protocol.hpp"
using namespace Proto;

/** Server socket reimplementation. */
class ServerSocket : public Socket
{
   public:

   ServerSocket(int fd = -1);
   ~ServerSocket();

   /** Run event loop and process client requests.
     */
   void run();

   protected:

   /** Handle incoming data.
     * \param fd
     */
   bool read(int fd);

   /** Handle incoming packet.
     * \param fd source fd
     * \param pkt incoming packet
     * \todo Implement incoming packet shared buffer.
     */
   virtual bool handle(int fd, Packet& pkt) = 0;

   private:

   /* Opaque pointer */
   class Private;
   Private* d;
};

#endif // __serversocket_hpp__
/** @} */
