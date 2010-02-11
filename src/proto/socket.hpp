/***************************************************************************
 *   Copyright (C) 2010 Marek Vavrusa <xvavru00@stud.fit.vutbr.cz          *
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

#pragma once
#ifndef __socket_hpp__
#define __socket_hpp__
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>

/** C++ style wrapper for BSD sockets with state checking and error control.
  */
class Socket
{
   public:

   Socket(int fd = -1);
   virtual ~Socket();

   // Error enumeration
   enum {
      IOError        = -128,
      BadAddr,
      ConnectError,
      CloseError,
      SendError,
      RecvError,
      NotOpen,
      Ok             = 0
   } State;

   // Address enumeration
   enum {
      Local,
      All
   } Addr;

   // Connect to remote host:port
   int connect(std::string host, int port);

   // Listen on given port
   int listen(int port, int addr = All, int limit = 5);

   // Accept new connection
   int accept();

   // Close connection
   int close();

   // Send raw data
   int send(const char* buf, size_t size);

   // Receive raw data
   int recv(char* dst, int size, int flags = 0) {
      return ::recv(mSock, dst, size, flags);
   }

   // Returns whether is socket associated
   bool isOpen() { return mSock != -1; }

   // Returns socket id
   int sock() { return mSock; }

   // Returns port
   int port() { return mPort; }

   // Returns host as std::string reference
   const std::string& host() { return mHost; }

   // Return address as struct
   sockaddr_in& addr() { return mAddr; }

   protected:

   // Create TCP sockets
   int create();

   // Bind to port
   int bind(int port, int addr = All);

   private:

   int mSock;
   int mPort;
   sockaddr_in mAddr;
   std::string mHost;
};

#endif // __socket_hpp__
