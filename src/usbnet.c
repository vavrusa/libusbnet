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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usbnet.h"

/* Helper function to return only base name from path. */
static const char* filename(const char* path) {
   const char* p = strrchr(path, '/');

   if(p == NULL)
      return path;

   return ++p;
}

/** Debug macro for marking unimplemented calls. */
#ifdef DEBUG
#define NOT_IMPLEMENTED \
   fprintf(stderr, "[!!] %s:%d: function '%s' not implemented\n", filename(__FILE__), __LINE__, __func__);
#else
#define NOT_IMPLEMENTED
#endif

/** Load next symbol from linked shared libraries.
  * Return error if not found and exit with error state (1).
  */
#define READ_SYM(sym, ident) { \
   if((sym) == NULL) { \
      (sym) = dlsym(RTLD_NEXT, (ident)); \
      char* __res = dlerror(); \
      if (__res != NULL) { \
         fprintf(stderr, "[!!] %s:%d: READ_SYM(%s): %s\n", filename(__FILE__), __LINE__, (ident), __res); \
         exit(1);
      } \
   } \
}

void usb_init(void)
{
   static void (*func)(void) = NULL;
   LOAD_NEXT_SYM(func, "usb_init");
   NOT_IMPLEMENTED;
}

int usb_find_busses(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_busses");
   NOT_IMPLEMENTED;

   return 0;
}

int usb_find_devices(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_devices");
   NOT_IMPLEMENTED;

   return 0;
}

struct usb_bus* usb_get_busses(void)
{
   static struct usb_bus* (*func)(void) = NULL;
   READ_SYM(func, "usb_get_busses");
   NOT_IMPLEMENTED;

   return NULL;
}

