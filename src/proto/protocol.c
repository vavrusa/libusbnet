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
#include <assert.h>

Packet* pkt_new(uint32_t size, uint8_t op) {

   // Check packet size
   if(size == 0) {
      error_msg("%s: invalid packet size (size = 0)", __func__);
      return NULL;
   }

   // Alloc packet
   Packet* pkt = malloc(sizeof(Packet));
   pkt->buf = malloc(size * sizeof(char));
   pkt->bufsize = size;

   // Initialize packet
   pkt_init(pkt, op);
   return pkt;
}

void pkt_free(Packet* pkt) {
   free(pkt->buf);
   free(pkt);
}

void pkt_init(Packet* pkt, uint8_t op)
{
   // Set opcode and resize
   pkt->op = op;
   pkt->size = 0;
}

int pkt_reserve(Packet* pkt, uint32_t size)
{
   if(pkt->bufsize < size) {
      pkt->bufsize = size + BUF_FRAGLEN;
      if((pkt->buf = realloc(pkt->buf, pkt->bufsize)) == NULL)
         pkt->bufsize = 0;
   }

   return pkt->buf != NULL;
}

uint32_t pkt_recv(int fd, Packet* dst)
{
   // Prepare packet
   uint32_t size = 0;
   dst->size = 0;

   // Read packet header
   assert(pkt_reserve(dst, PACKET_MINSIZE));
   if((size = pkt_recv_header(fd, dst->buf)) == 0)
      return 0;

   // Parse packet header
   dst->size = 0;
   dst->op = dst->buf[0];
   unpack_size(dst->buf + 1, &dst->size);

   // Receive payload
   if(dst->size > 0) {

      // Check buffer size
      assert(pkt_reserve(dst, dst->size));
      if((dst->size = recv_full(fd, dst->buf, dst->size)) == 0)
         return 0;
   }

   #ifdef DEBUG
   //pkt_dump(dst->buf, dst->size);
   #endif

   // Return packet size
   return dst->size;
}

int pkt_send(Packet* pkt, int fd)
{
   #ifdef DEBUG
   //pkt_dump(pkt->buf, pkt->size);
   #endif

   // Send opcode
   char buf[PACKET_MINSIZE] = { pkt->op };
   send(fd, buf, 1, 0);

   // Send size
   int len = pack_size(pkt->size, buf);
   send(fd, buf, len, 0);

   // Send payload
   return send(fd, pkt->buf, pkt->size, 0);
}

int pkt_append(Packet* pkt, uint8_t type, uint16_t len, const void* val)
{
   // Reserve packet size
   uint16_t isize = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + len;
   assert(pkt_reserve(pkt, pkt->size + isize));
   char* dst = pkt->buf + pkt->size;

   // Write T-L-V
   *dst = type; dst += sizeof(uint8_t);
   *dst = 0x82; dst += sizeof(uint8_t);
   uint16_t wlen = htons(len);
   memcpy(dst, &wlen, sizeof(uint16_t));
   if(len > 0) {
      memcpy(dst + sizeof(uint16_t), val, len);
   }

   // Update packet size
   pkt->size += isize;
   return isize;
}

int pkt_addnumeric(Packet* pkt, uint8_t type, uint16_t len, int32_t val)
{
   // Cast to ensure correct data
   // Unsigned type is used just as byte-buffer
   uint8_t val8 = val;
   uint16_t val16 = htons((uint16_t) val);
   val = htonl(val);

   // Append as byte-array
   switch(len) {
   case sizeof(uint8_t):  pkt_append(pkt, type, len, (const void*) &val8); break;
   case sizeof(uint16_t): pkt_append(pkt, type, len, (const void*) &val16); break;
   case sizeof(uint32_t): pkt_append(pkt, type, len, (const void*) &val); break;
   default:
      return 0;
      break;
   }

   return len;
}

void* pkt_begin(Packet* pkt, Iterator* it)
{
   // Invalidate ptrs
   it->type = InvalidType;
   it->len = 0;
   it->cur = it->next = it->end = NULL;

   // Check packet size
   it->next = pkt->buf;
   it->end = it->next + pkt->size;
   iter_next(it);

   return it->cur;
}

int iter_end(Iterator* it)
{
   return (it->cur >= it->end);
}

void* iter_next(Iterator* it)
{
   // Invalidate
   it->type = InvalidType;
   it->len = 0;

   // Check boundary
   if(it->next >= it->end) {
      it->cur = it->next;
      return NULL;
   }

   // Read type
   it->cur = it->next;
   char* p = it->cur;
   it->type = *((uint8_t*) p);
   ++p;

   // Read length
   it->len = 0;
   p += unpack_size(p, &it->len);

   // Read value
   it->val = p;

   // Save ptrs
   it->next = p + it->len;

   // Return current
   return it->cur;
}

void* iter_nextval(Iterator* it)
{
   void* val = it->val;
   iter_next(it);
   return val;
}

void* iter_enter(Iterator* it)
{
   // Get symbol header size
   uint32_t bsize;
   int len = unpack_size(it->cur + 1, &bsize);

   // Shift by header size and use as next
   it->next = it->cur + 1 + len;
   return iter_next(it);
}

/** @} */
