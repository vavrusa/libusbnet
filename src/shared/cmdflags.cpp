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

#include "cmdflags.hpp"
#include <cstdio>
#include <cstring>

CmdFlags::CmdFlags(int argc, char* argv[])
   : mPos(1)
{
   mArgs.assign(argv, argv + argc);
}

CmdFlags::Match CmdFlags::getopt() {

   // Check boundaries
   Match m(-1, "");
   if(mPos >= mArgs.size())
      return m;

   // Get current arg
   const char* pin = mArgs[mPos++];
   if(strlen(pin) > 1) {
      std::vector<Option>::iterator it;
      for(it = mOptions.begin(); it != mOptions.end(); ++it) {

         // Found match
         char code[3] = { '-', it->code, 0 };
         if(strncmp(pin, code, 2) == 0 || (strcmp(pin + 2, it->name) == 0)) {
            m.first = it->code;
            break;
         }
      }
   }

   // Shift
   if(pin[0] == '-') {

      // Check match
      if(m.first < 0) {
         fprintf(stderr, "getopt(): invalid parameter '%s'\n", pin);
      }

      // Add value to valid parameter
      if(mPos < mArgs.size())
         m.second = mArgs[mPos];

      ++mPos;
   }
   else {
      m.first = 0;
      m.second = pin;
   }

   return m;
}

void CmdFlags::printHelp() {

   // Print usage
   if(!mUsage.empty())
      printf("%s\n", mUsage.c_str());

   // Print options
   std::vector<Option>::iterator it;
   for(it = mOptions.begin(); it != mOptions.end(); ++it) {
      printf("  -%c,--%s\t .... %s (default '%s').\n",
             it->code, it->name, it->desc, it->implicit);
   }
}
