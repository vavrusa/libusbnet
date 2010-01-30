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

#include "serversocket.hpp"
#include <iostream>
#include <vector>
#include <cstdio>
#include <sys/poll.h>
using std::cout;

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
   cout << "Running server at " << host() << ":" << port() << " ...\n";

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
            cout << "Connected (fd " << it->fd << ").\n";
            d->clients.push_back(*it);
         }
         incoming.clear();
      }

      // Poll clients
      // Contiguity for std::vector is mandated by the standard [See 23.2.4./1]
      if(poll(&d->clients[0], d->clients.size(), 2000) > 0)
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
                  cout << "Incoming data (fd " << it->fd << ").\n";
                  if(!handle(it->fd))
                     it->revents |= POLLHUP;
               }
            }

            // Disconnect
            if(it->revents & POLLHUP) {
               cout << "Disconnected (fd " << it->fd << ").\n";
               d->clients.erase(it);
               it = d->clients.begin();
               continue;
            }
         }
      }
   }

   // Stop server
   cout << "Stopping server.\n";
}

bool ServerSocket::handle(int fd)
{
   Socket c(fd);
   char buf[256 + 2] = {0};
   char* dst = buf;
   int rcvd = 0, pending = 2;

   while(pending != 0) {
      rcvd = c.recv(dst, pending);
      fprintf(stderr, "Read: %d pending %d (val 0x%x).\n", rcvd, pending, *dst);
      if(rcvd <= 0)
         return false;
      pending -= rcvd;
      dst += rcvd;
   }

   pending = buf[1];
   printf("Call: 0x%x expected length: %d B\n", buf[0], pending);

   while(pending != 0) {
      rcvd = c.recv(dst, pending);
      fprintf(stderr, "Read: %d pending %d.\n", rcvd, pending);
      if(rcvd <= 0)
         return false;
      pending -= rcvd;
      dst += rcvd;
   }

   printf("Call: 0x%x loaded payload.\n", buf[0]);

   return true;
}
