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
/*! \file clientsocket.hpp
    \brief Client socket implementation.
    \author Marek Vavrusa <marek@vavrusa.com>

    Implements authentication and SSH tunnelling.

    \addtogroup client
    @{
  */
#pragma once
#ifndef __clientsocket_hpp__
#define __clientsocket_hpp__
#include "socket.hpp"
#include <string>

/** Client socket reimplementation. */
class ClientSocket : public Socket
{
   public:

   /** Authentication methods.
     */
   enum Auth {
      None = 0,
      SSH,
   };

   ClientSocket(int fd = -1, Auth method = None);
   ~ClientSocket();

   /** Return used authentication method.
     */
   Auth method();

   /** Set authentication mechanism.
     */
   void setMethod(Auth method);

   /** Set credentials for underlying authentication mechanism.
     */
   bool setCredentials(std::string auth);

   /** Connection timeout.
     */
   int timeout();

   /** Set connection timeout.
     */
   void setTimeout(int ms);

   /** Overload connect method.
     */
   int connect(std::string host, int port);

   /** Overload close method.
     */
   int close();

   private:

   /* Opaque pointer */
   class Private;
   Private* d;
};

#endif // __clientsocket_hpp__
/** @} */
