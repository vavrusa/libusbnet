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
/*! \file usbexportd.cpp
    \brief Server executable.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup server
    @{
  */
#include "usbservice.hpp"
#include "cmdflags.hpp"
#include "common.h"
#include <csignal>
#include <cstdlib>

// Global service handler ptr
UsbService* sService = NULL;

// SIGINT signal handler
void interrupt_handle(int s)
{
   // Stop server
   if(s == SIGINT && sService != NULL) {
      sService->close();
   }
}

int main(int argc, char* argv[])
{
   // Command line options
   int host = ServerSocket::All;

   // Parse command line arguments
   CmdFlags cmd(argc, argv);
   cmd.add('l', "local", "Bind to localhost only.")
      .add('?', "help");

   cmd.setUsage("Usage: usbexportd [options]");

   // Parse command line arguments
   CmdFlags::Match m = cmd.getopt();
   while(m.first >= 0) {

      // Evaluate
      switch(m.first) {
      case 'l':
         log_msg("Server: binding to localhost only");
         host = ServerSocket::Local;
         break;

      case '?':
         cmd.printHelp();
         return EXIT_SUCCESS;
         break;

      default:
         break;
      }

      // Next option
      m = cmd.getopt();
   }

   // Create server socket
   UsbService service;
   if(service.listen(22222, host) != Socket::Ok) {
      return EXIT_FAILURE;
   }

   // Register service and signal handler
   sService = &service;
   struct sigaction sa;
   sa.sa_handler = interrupt_handle;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigaction(SIGINT, &sa, NULL);

   // Process client requests
   service.run();

   // Close socket
   if(service.close() != Socket::Ok) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
/** @} */
