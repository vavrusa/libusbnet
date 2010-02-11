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

#include "clientsocket.hpp"
#include "common.h"
#include "cmdflags.hpp"
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[])
{
   // Create remote connection
   ClientSocket remote;
   std::string host("localhost"), auth, lib("libusbnet.so"), exec;
   int port = 22222, pos = 0, timeout = 1000;

   // Parse command line arguments
   CmdFlags cmd(argc, argv);
   cmd.add('h', "host",     "Target server host:[port]", "localhost:22222")
      .add('a', "auth",     "Authentication token user@host[:port]")
      .add('l', "library",  "Preloaded library", "libusbnet.so")
      .add('t', "timeout",  "Connection timeout (ms).", "1000")
      .add('?', "help");

   cmd.setUsage("Usage: usbnet [options] <executable>");

   // Parse command line arguments
   CmdFlags::Match m = cmd.getopt();
   while(m.first >= 0) {

      // Evaluate
      switch(m.first) {
      case 'h':
         host = m.second;
         pos = host.find(':');
         if(pos != std::string::npos) {
            port = atoi(host.substr(pos + 1).c_str());
            host.erase(pos);
         }
         break;
      case 'a': auth    = m.second; break;
      case 'l': lib     = m.second; break;
      case 't': timeout = atoi(m.second.c_str()); break;
      case '?':
         cmd.printHelp();
         return EXIT_SUCCESS;
         break;
      case  0 :
         exec = m.second;
         break;
      default:
         break;
      }

      // Next option
      m = cmd.getopt();
   }

   // Empty executable
   if(exec.empty()) {
      cmd.printHelp();
      return EXIT_FAILURE;
   }

   // Authenticate
   if(!auth.empty()) {
      remote.setMethod(ClientSocket::SSH);
      remote.setTimeout(timeout);
      if(!remote.setCredentials(auth)) {
         error_msg("Client: invalid authentication method '%s'", auth.c_str());
         cmd.printHelp();
         return EXIT_FAILURE;
      }

   }

   // Connect
   log_msg("Client: connecting to %s:%d ...", host.c_str(), port);
   if(remote.connect(host.c_str(), port) != Socket::Ok) {
      error_msg("Client: connection failed.");
      remote.close();
      return EXIT_FAILURE;
   }

   // Disable TCP buffering
   int flag = 1;
   setsockopt(remote.sock(), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

   // Create SHM segment
   int shm_id = 0;
   log_msg("IPC: creating segment at key 0x%x (%d bytes)", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT|0666)) == -1) {
      perror("shmget");
      return EXIT_FAILURE;
   }

   // Attach segment and save fd
   log_msg("IPC: attaching segment %d", shm_id);
   void* shm_addr = NULL;
   if ((shm_addr = shmat(shm_id, NULL, 0)) == (void *) -1) {
      perror("shmat");
      return EXIT_FAILURE;
   }

   // Save fd
   int* shm = (int*) shm_addr;
   *shm = remote.sock();
   log_msg("IPC: socket %d is stored in %d (mapped to %p)", remote.sock(), shm_id, shm);

   // Detach segment
   log_msg("IPC: detaching segment %d", shm_id);
   if(shmdt(shm_addr) != 0) {
      perror("shmdt");
      return EXIT_FAILURE;
   }

   // Run executable with preloaded library
   std::string execs("LD_PRELOAD=\"");
   execs.append(lib);
   execs.append("\" ");
   execs.append(exec);
   log_msg("Executing: %s", execs.c_str());
   std::string fill = "Executing: " + execs;
   fill = std::string(fill.length(), '-');
   log_msg(fill.c_str());

   int ret = system(execs.c_str());
   log_msg(fill.c_str());
   log_msg("IPC: executable returned %d", ret);

   // Delete segment
   log_msg("IPC: removing segment %d", shm_id);
   shmctl(shm_id, IPC_RMID, NULL);

   // Close socket
   if(remote.close() != Socket::Ok) {
      return EXIT_FAILURE;
   }

   return ret;
}
