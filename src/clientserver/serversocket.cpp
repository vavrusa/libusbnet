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
   std::vector<pollfd> clients;
};

ServerSocket::ServerSocket(int fd)
   : Socket(fd), d(new Private)
{
   // Append self to clients vector
   pollfd self;
   self.events = POLLIN; // Only reading
   self.fd = sock();     // Server fd
   d->clients.push_back(self);
}

ServerSocket::~ServerSocket()
{
   delete d;
}

void ServerSocket::run()
{
   cout << "Running server at " << host() << ":" << port() << " ...\n";

   // Process event loop
   while(isOpen()) {

      // Poll clients
      // Contiguity for std::vector is mandated by the standard [See 23.2.4./1]
      while(poll(&d->clients[0], d->clients.size(), 500) > 0)
      {
         // Check server for read
         std::vector<pollfd>::iterator it;
         for(it = d->clients.begin(); it != d->clients.end(); ++it) {

            // Incoming
            if(it->revents & POLLIN) {

               // Server socket
               if(it == d->clients.begin()) {
                  pollfd client;
                  client.events = POLLIN|POLLOUT;
                  client.fd = accept();
                  client.revents = 0;

                  // Accept client
                  if(client.fd > 0) {
                     d->clients.push_back(client);
                     cout << "Incoming connection to " << client.fd << ".\n";
                  }
               }
               else {
                  cout << "Incoming data from " << it->fd << ".\n";
               }
            }
         }
      }
   }

}
