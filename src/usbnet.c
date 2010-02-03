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
#include "protocol.h"

// Remote socket filedescriptor
static int __remote_fd = -1;

// Remote USB busses with devices
static struct usb_bus* __remote_bus = 0;

// Return remote filedescriptor
static int get_remote() {

   // Check fd
   if(__remote_fd != -1)
      return __remote_fd;

   // Get fd from SHM
   int shm_id = 0;
   printf("IPC: accessing segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, 0666)) != -1) {

      // Attach segment and read fd
      printf("IPC: attaching segment %d\n", shm_id);
      void* shm_addr = NULL;
      if ((shm_addr = shmat(shm_id, NULL, 0)) != (void *) -1) {

         // Read fd
         __remote_fd = *((int*) shm_addr);

         // Detach
         shmdt(shm_addr);
      }
   }

   // Check resulting fd
   printf("IPC: remote fd is %d\n", __remote_fd);
   if(__remote_fd == -1) {
      fprintf(stderr, "IPC: unable to access remote fd\n");
      exit(1);
   }

   return __remote_fd;
}

void usb_init(void)
{
   static void (*func)(void) = NULL;
   READ_SYM(func, "usb_init")

   // Initialize remote fd
   int fd = get_remote();

   // Create buffer
   char buf[PACKET_MINSIZE];
   packet_t pkt = pkt_create(buf, PACKET_MINSIZE);
   pkt_init(&pkt, UsbInit);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Call locally
   func();
}

int usb_find_busses(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_busses")

   // Get remote fd
   int fd = get_remote();

   // Create buffer
   char buf[32];
   packet_t pkt = pkt_create(buf, 32);
   pkt_init(&pkt, UsbFindBusses);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get number of changes
   int res = 0;
   sym_t sym;
   if(pkt_recv(fd, &pkt) > 0) {
      pkt_begin(&pkt, &sym);
      if(sym.cur != pkt_end(&pkt)) {
         if(sym.type == IntegerType)
            res = as_uint(sym.val, sym.len);

         printf("%s: returned %d\n", __func__, res);
      }
   }

   // Call locally
   return func() + res;
}

int usb_find_devices(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_devices")
   NOT_IMPLEMENTED

   // Get remote fd
   int fd = get_remote();

   // Create buffer
   char buf[4096];
   packet_t pkt = pkt_create(buf, 4096);
   pkt_init(&pkt, UsbFindDevices);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get number of changes
   int res = 0;
   sym_t sym;
   if(pkt_recv(fd, &pkt) > 0) {
      pkt_begin(&pkt, &sym);

      // Get return value
      if(sym.type == IntegerType) {
         res = as_uint(sym.val, sym.len);
         printf("%s: returned %d\n", __func__, res);
      }

      // Get busses
      for(;;) {

         // Evaluate
         if(sym.type == OctetType)
            printf("* string value: %s\n", as_string(sym.val, sym.len));
         if(sym.type == IntegerType)
            printf("* int value: %u\n", as_uint(sym.val, sym.len));

         if(sym.next == pkt_end(&pkt))
            break;

         // Check symbol
         if(sym.type & StructureType) {
            printf("<entering structure 0x%02x>\n", sym.type);
            sym_enter(&sym);
         }
         else
            sym_next(&sym);

      }
   }

   // Call locally
   return func() + res;
}

struct usb_bus* usb_get_busses(void)
{
   static struct usb_bus* (*func)(void) = NULL;
   READ_SYM(func, "usb_get_busses")
   NOT_IMPLEMENTED

   // TODO: merge both Local/Remote bus in future

   return __remote_bus;
}

