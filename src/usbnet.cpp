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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include "usbnet.h"
#include "common.h"

// Remote socket filedescriptor
static int __remote_fd = -1;

// Return remote filedescriptor
static int get_remote() {

   // Check fd
   if(__remote_fd != -1)
      return __remote_fd;

   // Get fd from SHM
   int shm_id = 0;
   printf("IPC: accessing segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, 0666)) == -1) {
      return __remote_fd;
   }

   // Attach segment and read fd
   printf("IPC: attaching segment %d\n", shm_id);
   void* shm_addr = NULL;
   if ((shm_addr = shmat(shm_id, NULL, 0)) == (void *) -1) {
      return __remote_fd;
   }

   // Read fd
   __remote_fd = *((int*) shm_addr);

   // Detach
   shmdt(shm_addr);

   printf("IPC: remote fd is %d\n", __remote_fd);
   return __remote_fd;
}

void usb_init(void)
{
   static void (*func)(void) = NULL;
   READ_SYM(func, "usb_init")
   NOT_IMPLEMENTED

   int fd = get_remote();
   char buf[64] = "usb_init";
   send(fd, buf, strlen(buf));
}

int usb_find_busses(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_busses")
   NOT_IMPLEMENTED

   return 0;
}

int usb_find_devices(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_devices")
   NOT_IMPLEMENTED

   return 0;
}

struct usb_bus* usb_get_busses(void)
{
   static struct usb_bus* (*func)(void) = NULL;
   READ_SYM(func, "usb_get_busses")
   NOT_IMPLEMENTED

   return NULL;
}

