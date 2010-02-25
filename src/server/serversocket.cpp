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
/*! \file serversocket.cpp
    \brief Abstract server socket and packet handler.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup server
    @{
  */
#include "serversocket.hpp"
#include "common.h"
#include <sys/poll.h>

class ServerSocket::Private
{
   public:
   std::vector<struct pollfd> clients;
};

ServerSocket::ServerSocket(int fd)
   : Socket(fd), d(new Private)
{
}

ServerSocket::~ServerSocket()
{
   delete d;
}

void ServerSocket::run()
{
   log_msg("Server: running at %s:%d", host().c_str(), port());

   // Append self to clients vector
   std::vector<pollfd>::iterator it;
   std::vector<pollfd> incoming;
   struct pollfd self;
   self.events = POLLIN; // Only reading
   self.fd = sock();     // Server fd
   incoming.push_back(self);

   // Process event loop
   while(isOpen()) {

      // Evaluate incoming sockets
      if(!incoming.empty()) {
         for(it = incoming.begin(); it != incoming.end(); ++it) {
            if(it->fd == sock()) {
               log_msg("Server: listening on fd %d", it->fd);
            }
            else {
               log_msg("Server: client connected (socket fd %d)", it->fd);
            }
            d->clients.push_back(*it);
         }
         incoming.clear();
      }

      // Poll clients
      // Contiguity for std::vector is mandated by the standard [See 23.2.4./1]
      if(poll(&d->clients[0], d->clients.size(), 1000) > 0)
      {
         // Check server for read
         for(it = d->clients.begin(); it != d->clients.end(); ++it) {

            // Incoming
            if(it->revents & POLLIN) {

               // Server socket
               if(it->fd == d->clients[0].fd) {

                  struct pollfd client;
                  client.events = POLLIN;
                  client.fd = accept();
                  client.revents = 0;

                  // Accept client
                  if(client.fd > 0) {
                     incoming.push_back(client);
                  }
               }
               else {
                  if(!read(it->fd))
                     it->revents |= POLLHUP;
               }
            }

            // Disconnect
            if(it->revents & POLLHUP) {
               log_msg("Server: client disconnected (socket fd %d)", it->fd);
               d->clients.erase(it);
               it = d->clients.begin();
               continue;
            }
         }
      }
   }

   // Stop server
   log_msg("Server: stopped");
}

bool ServerSocket::read(int fd)
{
   Packet pkt;

   // Read packet
   if(pkt.recv(fd) < 0) {
      return false;
   }

   // Handle incoming packet
   handle(fd, pkt);

   return true;
}

/** @} */
