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
/*! \file protocol.c
    \brief Protocol definitions and packet handling in C.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup proto
    @{
  */
#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

packet_t* pkt_new(uint32_t size) {

   // Minimum packet size is PACKET_MINSIZE
   if(size < PACKET_MINSIZE) {
      fprintf(stderr, "%s: minimum packet size is %dB\n", __func__, PACKET_MINSIZE);
      size = PACKET_MINSIZE;
   }

   packet_t* pkt = malloc(sizeof(packet_t));
   pkt->buf = malloc(size * sizeof(char));
   pkt->bufsize = size;
   pkt->size = 0;
   return pkt;
}

void pkt_del(packet_t* pkt) {
   free(pkt->buf);
   free(pkt);
}

void pkt_init(packet_t* pkt, uint8_t op)
{
   // Clear
   memset(pkt->buf, 0, pkt->bufsize);

   // Set opcode
   pkt->buf[0] = (char) op;
   pkt->buf[1] = 0x84; //! \todo C API packet size packing.

   // Set data pos
   pkt->size = PACKET_MINSIZE;
}

uint32_t pkt_size(packet_t* pkt)
{
   return pkt->size;
}

int pkt_send(int fd, const char* buf, int size)
{
   // Send buffer
   int res = send(fd, buf, size, 0);
   return res;
}

uint32_t pkt_recv(int fd, packet_t* dst)
{
   // Prepare packet
   uint32_t size = 0;
   dst->size = 0;

   // Read packet header
   if((size = pkt_recv_header(fd, dst->buf)) == 0)
      return 0;
   dst->size += size;

   // Unpack payload length
   uint32_t pending = 0;
   int len = unpack_size(dst->buf + 1, &pending);
   char* ptr = dst->buf + 1 + len;

   // Receive payload
   if(pending > 0) {
      if((size = recv_full(fd, ptr, pending)) == 0)
         return 0;

      dst->size += size;
   }

   #ifdef DEBUG
   //pkt_dump(dst->buf, dst->size);
   #endif

   // Return packet size
   return dst->size;
}

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

int pkt_append(packet_t* pkt, uint8_t type, uint16_t len, void* val)
{
   char* dst = pkt->buf + pkt->size;

   // Write T-L-V
   uint16_t written = 0; char prefix = 0x82;
   memcpy(dst + written, &type, sizeof(char));    written += sizeof(char);
   memcpy(dst + written, &prefix, sizeof(char));  written += sizeof(char);
   memcpy(dst + written, &len, sizeof(uint16_t)); written += sizeof(uint16_t);
   if(len > 0) {
      memcpy(dst + written, val, len);
      written += len;
   }

   // Update write pos and packet size
   pkt->size += written;
   uint32_t nsize = pkt->size - PACKET_MINSIZE;
   memcpy(pkt->buf + 2, &nsize, sizeof(uint32_t));
   return written;
}

void pkt_begin(packet_t* pkt, sym_t* sym)
{
   // Invalidate symbol
   sym->type = InvalidType;
   sym->len = 0;

   // Check packet size
   if(pkt_size(pkt) > 1) {
      uint32_t pktsize;
      int len = unpack_size(pkt->buf + 1, &pktsize);
      sym->next = pkt->buf + 1 + len;
      sym->cur = NULL;
      sym_next(sym);
   }
   else
      sym->cur = pkt_end(pkt);
}

void* pkt_end(packet_t* pkt)
{
   return pkt->buf + pkt->size;
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

void* sym_next(sym_t* sym)
{
   // Invalidate
   sym->type = InvalidType;
   sym->len = 0;

   // Read type
   sym->cur = sym->next;
   char* p = sym->cur;
   sym->type = *((uint8_t*) p);
   ++p;

   // Read length
   sym->len = 0;
   p += unpack_size(p, &sym->len);

   // Read value
   sym->val = p;

   // Save ptrs
   sym->next = p + sym->len;

   // Return current
   return sym->cur;
}

void* sym_enter(sym_t* sym)
{
   // Get symbol header size
   uint32_t bsize;
   int len = unpack_size(sym->cur + 1, &bsize);

   // Shift by header size and use as next
   sym->next = sym->cur + 1 + len;
   return sym_next(sym);
}

unsigned as_uint(void* data, uint32_t bytes)
{
   switch(bytes) {
   case 1: return *((uint8_t*) data); break;
   case 2: return *((uint16_t*) data); break;
   case 4: return *((uint32_t*) data); break;
   default: break;
   }

   // Invalid number of bytes
   return 0;
}

int as_int(void* data, uint32_t bytes)
{
   switch(bytes) {
   case 1: return *((int8_t*) data); break;
   case 2: return *((int16_t*) data); break;
   case 4: return *((int32_t*) data); break;
   default: break;
   }

   // Invalid number of bytes
   return 0;
}

const char* as_string(void* data, uint32_t bytes)
{
   return (const char*) data;
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
      uint16_t vval = val;
      dst[0] = (0x80 + sizeof(uint16_t));
      memcpy(dst + 1, (const char*) &vval, sizeof(uint16_t));
      return sizeof(uint16_t) + 1;
   }

   // 32bit value
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
      *dst = *((uint16_t*) (src + 1));
      return sizeof(uint16_t) + 1;
   }

   // 32bit value
   if(c == 0x84) {
      *dst = *((uint32_t*) (src + 1));
      return sizeof(uint32_t) + 1;
   }

   return 0;
}
/** @} */
