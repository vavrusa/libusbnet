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

#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

packet_t* pkt_new(int size) {

   // Minimum packet size is PACKET_MINSIZE
   if(size < PACKET_MINSIZE) {
      fprintf(stderr, "%s: minimum packet size is %dB\n", __func__, PACKET_MINSIZE);
      size = PACKET_MINSIZE;
   }

   packet_t* pkt = malloc(sizeof(packet_t));
   pkt->buf = malloc(size * sizeof(char));
   pkt->size = size;
   return pkt;
}

void pkt_del(packet_t* pkt) {
   free(pkt->buf);
   free(pkt);
}

void pkt_init(packet_t* pkt, Type op)
{
   // Clear
   memset(pkt->buf, 0, pkt->size);

   // Set opcode
   pkt->buf[0] = (char) op;

   // Set data pos
   pkt->pos = PACKET_MINSIZE;
}

int pkt_size(packet_t* pkt)
{
   return pkt->pos;
}

int pkt_send(int fd, const char* buf, int size)
{
   // Send buffer
   int res = send(fd, buf, size, 0);
   printf("%s(%d, size %d) = %d\n", __func__, fd, size, res);
   return res;
}

int pkt_recv(int fd, packet_t* dst)
{
   // Prepare
   char* p = dst->buf;
   int rcvd = 0; uint16_t pending = PACKET_MINSIZE;

   // Read packet header
   pkt_recv_header(fd, dst->buf);

   // Read packet payload
   dst->pos = PACKET_MINSIZE;
   pkt_recv_payload(fd, dst->buf);

   // Handle incoming packet
   printf("Packet: loaded payload.\n");
   dst->pos += (uint16_t) *(dst->buf + 1);

   // DEBUG: dump
   #ifdef DEBUG
   pkt_dump(dst->buf, dst->pos);
   #endif

   // Return packet size
   return dst->pos;
}

int pkt_recv_header(int fd, char *buf)
{
   int rcvd = 0, total = 0;
   uint16_t pending = PACKET_MINSIZE;

   // Read packet header
   while(pending != 0) {
      rcvd = recv(fd, buf, pending, 0);
      printf("Read: %d pending %d (val 0x%x).\n", rcvd, pending, *buf);
      if(rcvd <= 0) {
         return -1;
      }
      pending -= rcvd;
      buf += rcvd;
      total += rcvd;
   }

   return total;
}

int pkt_recv_payload(int fd, char *buf)
{
   int rcvd = 0, total = 0;
   uint16_t pending = 0;

   pending = (uint16_t) *(buf + 1);
   buf += PACKET_MINSIZE;
   while(pending != 0) {
      rcvd = recv(fd, buf, pending, 0);
      if(rcvd <= 0)
         return -1;
      printf("Read: %d pending %d.\n", rcvd, pending);
      pending -= rcvd;
      buf += rcvd;
      total += rcvd;
   }

   return total;
}

int pkt_append(packet_t* pkt, Type type, uint16_t len, void* val)
{
   char* dst = pkt->buf + pkt->pos;

   // Write T-L-V
   uint16_t written = 0;
   memcpy(dst + written, &type, sizeof(char));    written += sizeof(char);
   memcpy(dst + written, &len, sizeof(uint16_t)); written += sizeof(uint16_t);
   memcpy(dst + written, val, len);               written += len;
   printf("Packet: appended type: 0x%x len: %d total: %d\n", type, len, written);

   // Update write pos and packet size
   pkt->pos += written;
   uint16_t nsize = pkt->pos - PACKET_MINSIZE;
   memcpy(pkt->buf + 1, &nsize, sizeof(uint16_t));
   return written;
}

void pkt_begin(packet_t* pkt, param_t* param)
{
   if(pkt_size(pkt) > PACKET_MINSIZE)
      *param = pkt->buf + PACKET_MINSIZE;
   else
      *param = pkt_end(pkt);
}

void* pkt_end(packet_t* pkt)
{
   return pkt->buf + pkt->pos;
}

void pkt_dump(const char* buf, int size)
{
   printf("Packet (%dB):\n", size);
   int i = 0;
   for(i = 0; i < size; ++i) {
      printf("0x%02x ", (char) buf[i]);
   }
   if((size - 1) % 8 != 0)
      printf("\n");
}
