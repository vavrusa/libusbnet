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

#include "socket.hpp"
#include <cstring>
#include <iostream>
#include <sstream>
#include <netdb.h>

using namespace std;

Socket::Socket(int fd)
   : mSock(fd), mPort(0)
{
}

Socket::~Socket()
{
}

int Socket::create()
{
   // Create socket
   mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   // Reuse open socket
   int state = 1;
   if(setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (const char*) &state, sizeof(state)) < 0)
      return -1;

   return 0;
}

int Socket::connect(const std::string& host, int port)
{
   // Ignore multiple connect
   if(isOpen())
      return ConnectError;

   // Create socket
   hostent* hent = 0;
   if((hent = gethostbyname(host.c_str())) == 0)
      return BadAddr;

   if(create() < 0)
      return IOError;

   // Connect to host
   bzero(&mAddr, sizeof(mAddr));
   mAddr.sin_family = AF_INET;
   mAddr.sin_port = htons(port);
   memcpy(&mAddr.sin_addr, hent->h_addr, hent->h_length);
   if(::connect(mSock, (sockaddr*) &mAddr, sizeof(mAddr)) < 0) {
      return ConnectError;
   }

   // Done
   mHost = host;
   mPort = port;
   return Ok;
}

int Socket::send(const char* buf, size_t size)
{
   if(!isOpen())
      return NotOpen;

   int sent = 0;
   if((sent = ::send(mSock, buf, size, 0)) < 0)
      return SendError;

   return sent;
}

int Socket::listen(int port, int limit)
{
   // Create socket
   if(create() < 0)
      return IOError;

   // Bind to given port
   if(bind(port) < 0)
      return IOError;

   // Listen
   if(::listen(mSock, limit) < 0)
      return IOError;

   return Ok;
}

int Socket::accept()
{
   sockaddr_in client_addr;
   socklen_t client_addr_size;
   client_addr_size = sizeof(struct sockaddr_in);
   int client = ::accept(sock(), (sockaddr*) &client_addr, &client_addr_size);
   return client;
}

int Socket::bind(int port)
{
   // Create address
   mAddr.sin_family = AF_INET; // Addr type
   mAddr.sin_port = htons(port); // Set port
   mAddr.sin_addr.s_addr = INADDR_ANY; // First available addr

   if(::bind(mSock, (sockaddr*) &mAddr, sizeof(mAddr)) < 0)
      return -1;

   mHost = "localhost";
   mPort = port;
   return 0;
}


int Socket::close()
{
   // Close open socket
   if(isOpen()) {
      if(::close(mSock) < 0)
         return IOError;

      mSock = -1;
   }

   return Ok;
}
