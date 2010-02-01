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

#include "usbservice.hpp"
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

int main(int argc, char* argv[])
{
   // Create server socket
   UsbService service;
   if(service.listen(22222) != Socket::Ok) {
      return EXIT_FAILURE;
   }

   // Process client requests
   service.run();

   // Close socket
   if(service.close() != Socket::Ok) {
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
