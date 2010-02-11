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

#include "clientsocket.hpp"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

/* Portable sleep().
 */
#define msleep(ms) { \
  struct timeval tv; \
  tv.tv_sec = ms / 1000; \
  tv.tv_usec = (ms % 1000) * 1000; \
  select(0,NULL,NULL,NULL, &tv); \
}

/* Portable popen() alternative returning child pid.
 */
static pid_t popen2(const char *command);

/* Value to string converter.
 */
template <class T>
inline std::string to_string (const T& t)
{
   std::stringstream ss; ss << t; return ss.str();
}

class ClientSocket::Private
{
   public:
   std::string user;
   ClientSocket::Auth method;
   pid_t tunnel;
   int timeout;
};

ClientSocket::ClientSocket(int fd, Auth method)
   : Socket(fd), d(new Private)
{
   d->method = method;
   d->tunnel = -1;
   d->timeout = 0;
}

ClientSocket::~ClientSocket()
{
   delete d;
}

ClientSocket::Auth ClientSocket::method() {
   return d->method;
}

void ClientSocket::setMethod(Auth method) {
   d->method = method;
}

void ClientSocket::setCredentials(const std::string& user)
{
   d->user = user;
}

int ClientSocket::timeout()
{
   return d->timeout;
}

void ClientSocket::setTimeout(int ms)
{
   d->timeout = ms;
}

int ClientSocket::connect(std::string host, int port)
{
   // Prepare
   d->tunnel = -1;

   // Check auth method
   if(method() == SSH) {

      // Build command
      std::string cmd("ssh -o PreferredAuthentications=publickey ");

      // Host
      if(!d->user.empty()) {
         cmd += d->user + '@';
      }
      cmd += host;
      cmd += " -T -L "; // Do not allocate TTY, Tunnel mode

      // Tunnel options
      cmd += to_string(port + 1);
      cmd += ':';
      cmd += host;
      cmd += ':';
      cmd += to_string(port);
      cmd += " -N"; // Don't execute command

      // Redirect target connection
      host = "localhost";
      port = port + 1;

      // Execute
      printf("Client: creating secure SSH tunnel to %s@%s ...\n", d->user.c_str(), host.c_str());
      if((d->tunnel = popen2(cmd.c_str())) < 0)
         return -1;

      // Write password
      printf("Client: created on pid %d\n", d->tunnel);
      if(d->timeout > 0)
         msleep(d->timeout);
   }

   return Socket::connect(host, port);
}

int ClientSocket::close()
{
   // Kill tunnel if present
   if(d->tunnel > 0) {
      kill(d->tunnel, SIGTERM);
      waitpid(d->tunnel, NULL, 0);
      printf("Client: closed tunnel pid %d\n", d->tunnel);
      d->tunnel = -1;
   }

   return Socket::close();
}

/* popen() alternative, returns child process pid.
 */
pid_t popen2(const char *command)
{
    pid_t pid = vfork();

    if (pid < 0)
        return pid;

    if (pid == 0)
    {
        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(1);
    }

    return pid;
}

