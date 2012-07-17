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
/*! \file clientsocket.cpp
    \brief Client socket implementation.
    \author Marek Vavrusa <marek@vavrusa.com>

    Implements authentication and SSH tunnelling.

    \addtogroup client
    @{
  */
#include "clientsocket.hpp"
#include "common.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

/** Portable sleep() macro.
  * \param ms Sleep time in miliseconds.
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
   ClientSocket::Auth method;

   pid_t tunnel;
   std::string tunUser;
   std::string tunHost;
   int tunPort;
   int timeout;
};

ClientSocket::ClientSocket(int fd, Auth method)
   : Socket(fd), d(new Private)
{
   d->method = method;
   d->tunnel = -1;
   d->timeout = 0;
   d->tunPort = 22;
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

bool ClientSocket::setCredentials(std::string auth)
{
   // Prepare
   d->tunUser.clear();
   d->tunHost.clear();


   // Find username
   int pos = auth.find('@');
   if(pos != std::string::npos) {
      d->tunUser = auth.substr(0, pos);
      ++pos;
   }
   else
      pos = 0;

   // Find port
   int hlen = auth.length() - pos;
   int ppos = auth.find(':');
   if(ppos != std::string::npos) {
      std::string port = auth.substr(ppos + 1);
      hlen -= port.length() + 1;
      d->tunPort = atoi(port.c_str());
      if(d->tunPort <= 0 || d->tunPort > 65535) {
         error_msg("Client: invalid port given: %s", port.c_str());
         return false;
      }
   }

   // Get hostname
   if(hlen > 0)
      d->tunHost = auth.substr(pos, hlen);

   return true;
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

      // Username
      if(!d->tunUser.empty()) {
         cmd += d->tunUser + '@';
      }

      // Hostname
      if(d->tunHost.empty()) {
         d->tunHost = host;
      }

      cmd += d->tunHost;
      cmd += " -p " + to_string(d->tunPort);
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
      log_msg("Client: creating secure SSH (%s@%s:%d) ...", d->tunUser.c_str(), d->tunHost.c_str(), d->tunPort);
      if((d->tunnel = popen2(cmd.c_str())) < 0)
         return -1;

      // Write password
      log_msg("Client: created on pid %d", d->tunnel);
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
/** @} */
