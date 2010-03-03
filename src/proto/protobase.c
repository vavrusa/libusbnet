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
/*! \file protobase.c
    \brief Protocol definitions and base functions.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup proto
    @{
  */
#include "protobase.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <netinet/in.h>

uint32_t recv_full(int fd, char* buf, uint32_t pending)
{

   // Read packet header
   int rcvd = 0;
   uint32_t read = 0;
   while(pending != 0) {

      if((rcvd = recv(fd, buf, pending, 0)) <= 0)
         return 0;

      pending -= rcvd;
      buf += rcvd;
      read += rcvd;
   }

   return read;
}

uint32_t pkt_recv_header(int fd, char *buf)
{
   // Read packet header
   uint32_t rcvd = 0;
   if((rcvd = recv_full(fd, buf, 2)) == 0)
      return 0;

   // Multi-byte
   unsigned c = (unsigned char) buf[1];
   if(c > 0x80) {
      if((rcvd = recv_full(fd, buf + 2, c - 0x80)) == 0)
         return 0;
      rcvd += 2;
   }

   return rcvd;
}

void pkt_dump(const char* buf, uint32_t size)
{
   printf("Packet (%dB):", size);
   uint32_t i = 0;
   for(i = 0; i < size; ++i) {
      if(i == 0 || i % 8 == 0) {
         if(i == 160) {
            printf("\n ...");
            break;
         }
         printf("\n%4d | ", i);
      }
      printf("0x%02x ", (unsigned char) *(buf + i));
   }
   printf("\n");
}

int pack_size(uint32_t val, char* dst)
{
   // 8bit value
   if(val <= 0x80) {
      dst[0] = (unsigned char) val;
      return sizeof(char);
   }

   // 16bit value
   if(val < ((uint16_t)~0)) {
      uint16_t vval = htons((uint16_t) val);
      dst[0] = (0x80 + sizeof(uint16_t));
      memcpy(dst + 1, (const char*) &vval, sizeof(uint16_t));
      return sizeof(uint16_t) + 1;
   }

   // 32bit value
   val = htonl(val);
   dst[0] = (0x80 + sizeof(uint32_t));
   memcpy(dst + 1, (const char*) &val, sizeof(uint32_t));
   return sizeof(uint32_t) + 1;
}

int unpack_size(const char* src, uint32_t* dst)
{
   // 8bit value
   unsigned char c = (unsigned char) *src;
   if(c <= 0x80) {
      *dst = *((uint8_t*) (src));
      return sizeof(uint8_t);
   }

   // 16bit value
   if(c == 0x82) {
      *dst = ntohs(*((uint16_t*) (src + 1))) & 0x0000ffff;
      return sizeof(uint16_t) + 1;
   }

   // 32bit value
   if(c == 0x84) {
      *dst = ntohl(*((uint32_t*) (src + 1)));
      return sizeof(uint32_t) + 1;
   }

   return 0;
}

unsigned as_uint(void* data, uint32_t bytes)
{
   unsigned val = 0;
   switch(bytes) {
   case 1: val = *((uint8_t*) data); break;
   case 2: val = ntohs(*((uint16_t*) data)); break;
   case 4: val = ntohl(*((uint32_t*) data)); break;
   default: break;
   }

   return val;
}

int as_int(void* data, uint32_t bytes)
{
   int val = 0;
   switch(bytes) {
   case 1: val = *((int8_t*) data); break;
   case 2: val = ntohs(*((int16_t*) data)); break;
   case 4: val = ntohl(*((int32_t*) data)); break;
   default: break;
   }

   return val;
}

const char* as_string(void* data, uint32_t bytes)
{
   return (const char*) data;
}

int ipc_init()
{
   int shm_id = 0;
   log_msg("IPC: creating segment at key 0x%x (%d bytes)", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT|0666)) == -1) {
      perror("shmget");
   }

   return shm_id;
}

int ipc_teardown(int shm_id)
{
   log_msg("IPC: removing segment %d", shm_id);
   return shmctl(shm_id, IPC_RMID, NULL);
}

int ipc_get_remote()
{
   // Get fd from SHM
   int fd = 0;

   // Get fd from SHM
   int shm_id = 0;
   printf("IPC: accessing segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, 0666)) != -1) {

      // Attach segment and read fd
      printf("IPC: attaching segment %d\n", shm_id);
      void* shm_addr = NULL;
      if ((shm_addr = shmat(shm_id, NULL, 0)) != (void *) -1) {

         // Read fd
         fd = *((int*) shm_addr);

         // Detach
         shmdt(shm_addr);
      }
   }

   // Check resulting fd
   printf("IPC: remote fd is %d\n", fd);

   // Check peer name - keep-alive
   struct sockaddr_in addr;
   int len = sizeof(addr);
   if(getpeername(fd, (struct sockaddr*)& addr, &len) < 0)
      fd = -1;

   return fd;
}

int ipc_set_remote(int fd)
{
   // Save fd to SHM
   int shm_id = 0;
   printf("IPC: accessing segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
   if((shm_id = shmget(SHM_KEY, SHM_SIZE, 0666)) != -1) {

      // Attach segment and read fd
      printf("IPC: attaching segment %d\n", shm_id);
      void* shm_addr = NULL;
      if ((shm_addr = shmat(shm_id, NULL, 0)) != (void *) -1) {

         // Read fd
         *((int*) shm_addr) = fd;
         log_msg("IPC: remote socket descriptor %d is stored shm(%d:%p)", fd, shm_id, shm_addr);

         // Detach
         shmdt(shm_addr);
      }
   }

   return shm_id;
}

/** @} */
