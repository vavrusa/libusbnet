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
/*! \file protocol.h
    \brief Protocol definitions and packet handling in C.
    \author Marek Vavrusa <marek@vavrusa.com>

    Each entity is represented as symbol - structures and atomic types.
    Example:
       packet = 1 {     // 0x31 - Type, 0x06 - Length, is structural
         int(4B) 2;     // 0x02 - Type, 0x04 - Length, 0x02 0x00 0x00 0x00 - Value
       }
    How to parse:
       pkt_begin(&pkt, &sym); // Read packet size and enter
       if(sym.type == IntegerType)
         printf("int(4B): %d\n", as_int(sym.val, sym.len));

    \addtogroup proto
    @{
  */
#pragma once
#ifndef __protocol_h__
#define __protocol_h__
#include <stdint.h>
#include "protobase.h"

/** Packet structure. */
typedef struct {
   uint32_t bufsize;
   uint32_t size;
   char* buf;
} Packet;

/** Parameter structure. */
typedef struct {
   void *cur, *next, *end;
   uint8_t type;
   uint32_t len;
   void*    val;
} sym_t;

/** Packet manipulation interface.
 *  \todo Automatic packet size allocation.
 *  \todo Fixed size packets boundaries checking.
 */

/** Initialize packet on existing buffer.
  * \param buf must be char* or char[]
  * \param size real buffer size
  */
#define pkt_create(buf, size) { size, 0, buf }

/** Allocate new packet.
  * Minimal packet size is 1 + 4B.
  * \param size required packet size
  * \param op packet opcode
  * \return new packet
  */
Packet* pkt_new(uint32_t size, uint8_t op);

/** Free allocated packet.
  * Use only with dynamically allocated packets.
  * \param pkt freed packet
  */
void pkt_del(Packet* pkt);

/** Initialize packet.
  * \param pkt initialized packet
  * \param op packet opcode
  */
void pkt_init(Packet* pkt, uint8_t op);

/** Append parameter to packet.
  * \param pkt packet
  * \param type parameter type
  * \param len  parameter size
  * \param val  parameter value
  * \return bytes written
  */
int pkt_append(Packet* pkt, uint8_t type, uint16_t len, void* val);

/** Receive packet.
  * \param fd source fd
  * \param dst destination packet
  * \return packet size on success, 0 on error
  */
uint32_t pkt_recv(int fd, Packet* dst);

/** Return first symbol in packet.
  * \param pkt source packet
  * \param sym target symbol
  * \return current symbol or NULL
  */
void* pkt_begin(Packet* pkt, sym_t* sym);

/** Next symbol.
  * \return next symbol or NULL on end
  */
void* sym_next(sym_t* sym);

/** Step in current symbol (if structural type).
  * \return current symbol ptr
  */
void* sym_enter(sym_t* sym);

/** Interpret symbol as unsigned integer.
  */
unsigned as_uint(void* data, uint32_t bytes);

/** Interpres symbol as signed integer.
  */
int as_int(void* data, uint32_t bytes);

/** Interpret symbol as string.
  */
const char* as_string(void* data, uint32_t bytes);

#endif // __protocol_h__
/** @} */
