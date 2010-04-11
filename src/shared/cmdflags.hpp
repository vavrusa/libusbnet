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

#ifndef __cmdflags_hpp__
#define __cmdflags_hpp__
#include <vector>
#include <string>

/** Class representing single option. */
class Option
{
   public:
   Option(char _c, const char* _n, const char* _d = "", const char* _i = "", bool _v = true)
      : code(_c), name(_n), desc(_d), implicit(_i), has_value(_v) {
   }

   int         code;
   const char* name;
   const char* desc;
   const char* implicit;
   bool        has_value;
};

/** Simple class for portable C++ getopt().
  */
class CmdFlags
{
   public:

   /** Initialize command line arguments. */
   CmdFlags(int argc, char* argv[]);

   /** Add option. */
   CmdFlags& add(Option opt) {
      mOptions.push_back(opt);
      return *this;
   }

   /** Add option. */
   CmdFlags& add(char _c, const char* _n, const char* _d = "", const char* _i = "", bool _v = true) {
      mOptions.push_back(Option(_c, _n, _d, _i, _v));
      return *this;
   }


   /** Return options. */
   std::vector<Option>& options() {
      return mOptions;
   }

   /* Result match. */
   typedef std::pair<int, std::string> Match;

   /** Getopt interface. */
   Match getopt();

   /** Reset internal position. */
   void reset() {
      mPos = 0;
   }

   /** Set usage line. */
   void setUsage(std::string usage) {
      mUsage = usage;
   }

   /** Print usage help. */
   void printHelp();

   private:
   int mPos;
   std::vector<char*>  mArgs;
   std::vector<Option> mOptions;
   std::string mUsage;
};

#endif // __cmdflags_hpp__
