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

Packet* pkt_new(uint32_t size, uint8_t op) {

   // Minimum packet size is PACKET_MINSIZE
   if(size < PACKET_MINSIZE) {
      fprintf(stderr, "%s: minimum packet size is %ldB\n", __func__, PACKET_MINSIZE);
      size = PACKET_MINSIZE;
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
   // Clear
   memset(pkt->buf, 0, pkt->bufsize);

   // Set opcode
   pkt->buf[0] = (char) op;
   pkt->buf[1] = 0x84; //! \todo C API packet size packing.

   // Set data pos
   pkt->size = PACKET_MINSIZE;
}

uint32_t pkt_size(Packet* pkt)
{
   return pkt->size;
}

uint32_t pkt_recv(int fd, Packet* dst)
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

   // Assert
   if((pending + size) > dst->bufsize) {
      err_msg("packet bufsize too small, bufsize: %u packet size: %u.", dst->bufsize, pending + size);
      exit(1);
   }

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

int pkt_send(Packet* pkt, int fd)
{
   #ifdef DEBUG
   //pkt_dump(pkt->buf, pkt->size);
   #endif

   return send(fd, pkt->buf, pkt->size, 0);
}

/** \bug pkt_append does not convert byte-order of copied value, update pkt_addxyz family ..."
  */
#warning "pkt_append does not convert byte-order of copied value"
int pkt_append(Packet* pkt, uint8_t type, uint16_t len, const void* val)
{
   char* dst = pkt->buf + pkt->size;

   // Write T-L-V
   uint16_t wlen = htons(len);
   uint16_t written = 0; char prefix = 0x82;
   memcpy(dst + written, &type, sizeof(char));    written += sizeof(char);
   memcpy(dst + written, &prefix, sizeof(char));  written += sizeof(char);
   memcpy(dst + written, &wlen,sizeof(uint16_t)); written += sizeof(uint16_t);
   if(len > 0) {
      memcpy(dst + written, val, len);
      written += len;
   }

   // Update write pos and packet size
   pkt->size += written;
   uint32_t nsize = pkt->size - PACKET_MINSIZE;
   nsize = htonl(nsize);
   memcpy(pkt->buf + 2, &nsize, sizeof(uint32_t));
   return written;
}

void* pkt_begin(Packet* pkt, Iterator* it)
{
   // Invalidate symbol
   it->type = InvalidType;
   it->len = 0;
   it->cur = it->next = it->end = NULL;

   // Check packet size
   if(pkt_size(pkt) > 1) {
      uint32_t pktsize;
      int len = unpack_size(pkt->buf + 1, &pktsize);
      it->next = pkt->buf + 1 + len;
      it->end = it->next + pktsize;
      iter_next(it);
   }

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
