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
#ifndef __common_h__
#define __common_h__
#include <string.h>
#include <stdio.h>

/** Symbolic constants.
  */
#define SHM_KEY  (0x2a2a2a2a)
#define SHM_SIZE (512) // At least size of int, SHMMIN may be enforced (!)

/* Debugging and symbol loading macros.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/* Helper function to return only base name from path. */
static const char* filename(const char* path) {
   const char* p = strrchr(path, '/');

   if(p == NULL)
      return path;

   return ++p;
}

/** Log message.
  */
#define log_msg(fmt, args...) \
printf(fmt, ## args); \
printf("\n")

/** Error message.
  */
#define error_msg(fmt, args...) \
fprintf(stderr, fmt, ## args); \
fprintf(stderr, "\n")

/** Debug message.
  */
#ifdef DEBUG
#define debug_msg(fmt, args...) \
fprintf(stderr, "%s: ", __func__); \
fprintf(stderr, fmt, ## args); \
fprintf(stderr, "\n")

#else
#define debug_msg(fmt, args...)
#endif

/** Debug macro for marking unimplemented calls.
  */
#ifdef DEBUG
#define NOT_IMPLEMENTED \
   debug_msg("function not implemented (%s:%d)", filename(__FILE__), __LINE__);
#else
#define NOT_IMPLEMENTED
#endif

/** Debug macro for marking functions deprecated. */
#ifdef DEBUG
#ifdef __GNUC__
#define DEPRECATED __attribute__ ((deprecated))
#else
#define DEPRECATED
#endif
#endif

/* Deprecated functions / macros.
 */

#if 0
#include <dlfcn.h>

/** Load next symbol from linked shared libraries.
  * Return error if not found and exit with error state (1).
  */
#define READ_SYM(sym, ident) { \
   if((sym) == NULL) { \
      (sym) = dlsym(RTLD_NEXT, (ident)); \
      char* __res = dlerror(); \
      if (__res != NULL) { \
         fprintf(stderr, "[!!] %s:%d: READ_SYM(%s): %s\n", filename(__FILE__), __LINE__, (ident), __res); \
         exit(1); \
      } \
   } \
}
#endif

#ifdef __cplusplus
}
#endif

#endif // __common_h__
