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
    \addtogroup proto
    @{
  */
#pragma once
#ifndef __protocol_h__
#define __protocol_h__
#include "protobase.h"

/** \page proto_page
    <h2>Protocol C API</h2>
    Each entity is represented as Type-Length-Value.
    Example:
    \code
       packet = 1 {     // 0x31 - Type, 0x06 - Length, is structural
         int(4B) 2;     // 0x02 - Type, 0x04 - Length, 0x02 0x00 0x00 0x00 - Value
       }
    \endcode
    <h3>How to parse packet</h3>
    \code
       pkt_begin(&pkt, &it); // Read packet size and enter
       printf("int(4B): %d\n", iter_getint(&it)); // Return value as integer, move to next
    \endcode
    <h3>How to write packet</h3>
    \code
       char buf[PACKET_MINSIZE]; // Buffering on stack
       Packet pkt = pkt_create(buf, PACKET_MINSIZE); // Static initialization
       pkt_init(&pkt, UsbInit);  // Write packet header and opcode
       pkt_addint8(&pkt, 0x4F);  // Append 8bit integer
       pkt_adduint(&pkt, someval); // Append variable-length unsigned
       pkt_send(&pkt, fd);       // Send packet
    \endcode
  */


/// Default buffer increase
#define BUF_FRAGLEN 8

/** Packet structure. */
typedef struct {
   uint32_t bufsize;
   uint32_t size;
   char* buf;
} Packet;

/** Type-Length-Value representation. */
typedef struct {
   void *cur, *next, *end;
   uint8_t type;
   uint32_t len;
   void*    val;
} Iterator;

/** Packet manipulation interface.
 *  \todo Fixed size packets boundaries checking.
 */

/** Return packet opcode.
  */
#define pkt_op(pkt) ((pkt)->buf[0])

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
void pkt_free(Packet* pkt);

/** Reserve given size.
  * \param pkt given packet
  * \param size reserved buffer size
  * \return true or false if allocation fails
  */
int pkt_reserve(Packet* pkt, uint32_t size);

/** Initialize packet.
  * \param pkt initialized packet
  * \param op packet opcode
  */
void pkt_init(Packet* pkt, uint8_t op);

/** Append parameter to packet.
  * \warning No byte-order conversion applied, raw data copy only.
  * \param pkt packet
  * \param type parameter type
  * \param len  parameter size
  * \param val  parameter value
  * \return bytes written
  */
int pkt_append(Packet* pkt, uint8_t type, uint16_t len, const void* val);

/** Append numeric value. */
int pkt_addnumeric(Packet* pkt, uint8_t type, uint16_t len, int32_t val);

/** Add variable-length unsigned integer. */
#define pkt_adduint(pkt, val)   pkt_addnumeric((pkt), UnsignedType, sizeof(val), (val))

/** Add 8bit unsigned integer. */
#define pkt_adduint8(pkt, val)  pkt_addnumeric((pkt), UnsignedType, sizeof(uint8_t),  (val))

/** Add 16bit unsigned integer. */
#define pkt_adduint16(pkt, val) pkt_addnumeric((pkt), UnsignedType, sizeof(uint16_t), (val))

/** Add 32bit unsigned integer. */
#define pkt_adduint32(pkt, val) pkt_addnumeric((pkt), UnsignedType, sizeof(uint32_t), (val))

/** Add variable-length signed integer. */
#define pkt_addint(pkt, val)   pkt_addnumeric((pkt), IntegerType, sizeof(val), (val))

/** Add 8bit signed integer. */
#define pkt_addint8(pkt, val)  pkt_addnumeric((pkt), IntegerType, sizeof(int8_t),  (val))

/** Add 16bit signed integer. */
#define pkt_addint16(pkt, val) pkt_addnumeric((pkt), IntegerType, sizeof(int16_t), (val))

/** Add 32bit signed integer. */
#define pkt_addint32(pkt, val) pkt_addnumeric((pkt), IntegerType, sizeof(int32_t), (val))

/** Append string. */
#define pkt_addstr(pkt,len,val) pkt_append((pkt), OctetType, (len), (val))

/** Receive packet.
  * \param fd source fd
  * \param dst destination packet
  * \return packet size on success, 0 on error
  */
uint32_t pkt_recv(int fd, Packet* dst);

/** Send packet.
  * \param pkt given packet
  * \param fd destination socket descriptor
  * \return socket send() value
  */
int pkt_send(Packet* pkt, int fd);

/** Set iterator to first packet payload.
  * \param pkt source packet
  * \param it iterator
  * \return current item ptr or NULL
  */
void* pkt_begin(Packet* pkt, Iterator* it);


/** Return true on iterator end.
  * \return true if iterator is at lastpos + 1
  */
int iter_end(Iterator*);

/** Shift to next item.
  * \return next item ptr or NULL on error
  */
void* iter_next(Iterator* it);

/** Return current item value and shift to next item.
  * \return current item value
  */
void* iter_nextval(Iterator* it);

/** Step in current structure item.
  * \return next item ptr or NULL on error
  */
void* iter_enter(Iterator* it);

/** Return item value and move to next.
  * \param it iterator
  * \param convf conversion function ptr
  * \return item value casted to given type
  * \private
  */
#define iter_getval(it, convf) convf(iter_nextval((it)), it->len)

/** Return item as integer and move to next. */
#define iter_getint(it) (iter_getval((it), (&as_int)))

/** Return item as unsigned and move to next. */
#define iter_getuint(it) (iter_getval((it), (&as_uint)))

/** Return item as string and move to next. */
#define iter_getstr(it) (iter_getval((it), (&as_string)))

#endif // __protocol_h__
/** @} */
