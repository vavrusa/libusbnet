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
#include <sys/shm.h>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[])
{
   // Create remote connection
   // TODO: args
   ClientSocket remote;
   const char* host = "localhost";
   int port = 22222;
   printf("Connecting to %s:%d ...\n", host, port);
   if(remote.connect(host, port) != Socket::Ok) {
      printf("Connection failed.\n");
      return EXIT_FAILURE;
   }

   // Create SHM segment
   int shm_id = 0;
   printf("IPC: creating segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT|0666)) == -1) {
      perror("shmget");
      return EXIT_FAILURE;
   }

   // Attach segment and save fd
   printf("IPC: attaching segment %d\n", shm_id);
   void* shm_addr = NULL;
   if ((shm_addr = shmat(shm_id, NULL, 0)) == (void *) -1) {
      perror("shmat");
      return EXIT_FAILURE;
   }

   // Save fd
   int* shm = (int*) shm_addr;
   *shm = remote.sock();
   printf("IPC: socket %d is stored in %d (mapped to %p)\n", remote.sock(), shm_id, shm);

   // Detach segment
   printf("IPC: detaching segment %d\n", shm_id);
   if(shmdt(shm_addr) != 0) {
      perror("shmdt");
      return EXIT_FAILURE;
   }

   // Run executable with preloaded library
   if(argc > 1) {
      std::string cmd("LD_PRELOAD=\"");
      cmd.append(argv[1]);
      cmd.append("\" \"");
      cmd.append(argv[2]);
      cmd.append("\"");
      printf("Executing: %s\n", cmd.c_str());

      int ret = system(cmd.c_str());
      printf("IPC: executable returned %d\n", ret);
   }

   // Delete segment
   printf("IPC: removing segment %d\n", shm_id);
   shmctl(shm_id, IPC_RMID, NULL);

   // Close socket
   if(remote.close() != Socket::Ok) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
