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

#pragma once
#ifndef __protocol_h__
#define __protocol_h__
#include <stdint.h>

/** ASN.1 semantic types.
  *
  */
typedef enum {

   // Basic types
   InvalidType    = 0x00,
   BoolType       = 0x01,
   IntegerType    = 0x02,
   NullType       = 0x05,
   OctetType      = 0x04,
   SequenceType   = 0x10,
   EnumType       = 0x0A,
   SetType        = 0x11,
   StructureType  = 0x20,
   RawType        = StructureType + 1,
   CallType       = StructureType|SequenceType,

} Type;

/* Packet manipulation. */
#ifdef __cplusplus
extern "C"
{
#endif

/** Packet structure. */
typedef struct {
   uint32_t bufsize;
   uint32_t size;
   char* buf;
} packet_t;

/** Parameter structure. */
typedef struct {
   void *cur, *next;
   uint8_t type;
   uint32_t len;
   void*    val;
} sym_t;

/** Packet manipulation interface.
 *  TODO: Automatic allocation
 *        Fixed size packets size checking.
 */

// 1B op + 1B prefix + 4B length
#define PACKET_MINSIZE (sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint32_t))

/** Initialize packet on existing buffer.
  * \param buf must be char* or char[]
  * \param size real buffer size
  */
#define pkt_create(buf, size) { size, 0, buf }

/** Allocate new packet.
  * Minimal packet size is 1 + 4B.
  * \param size required packet size
  * \return new packet
  */
packet_t* pkt_new(uint32_t size);

/** Free allocated packet.
  * Use only with dynamically allocated packets.
  * \param pkt freed packet
  */
void pkt_del(packet_t* pkt);

/** Initialize packet.
  * \param pkt initialized packet
  * \param op packet opcode
  */
void pkt_init(packet_t* pkt, uint8_t op);

/** Append parameter to packet.
  * \param pkt packet
  * \param type parameter type
  * \param len  parameter size
  * \param val  parameter value
  * \return bytes written
  */
int pkt_append(packet_t* pkt, uint8_t type, uint16_t len, void* val);

/** Send packet.
  * \param fd destination filedescriptor
  * \param buf packet buffer
  * \param size packet buffer size
  * \return send() value
  */
int pkt_send(int fd, const char* buf, int size);

/** Receive packet.
  * \param fd source fd
  * \param pkt destination packet
  * \return packet size on success, 0 on error
  */
uint32_t pkt_recv(int fd, packet_t* dst);

/** Receive packet header.
  * Ensure buf is at least PACKET_MINSIZE.
  * \return header size on success, 0 on error
  */
uint32_t pkt_recv_header(int fd, char* buf);

/** Block until all pending data is received.
  */
uint32_t recv_full(int fd, char* buf, uint32_t pending);


/** Packet parsing interface.
 * Each entity is represented as symbol - structures and atomic types.
 * Example:
 *   packet = 1 {     // 0x31 - Type, 0x06 - Length, is structural
 *     int(4B) 2;     // 0x02 - Type, 0x04 - Length, 0x02 0x00 0x00 0x00 - Value
 *   }
 * How to parse:
 *   pkt_begin(&pkt, &sym); // Read packet size and enter
 *   if(sym.type == IntegerType)
 *     printf("int(4B): %d\n", as_int(sym.val, sym.len));
 */

/** Return first symbol in packet.
  * \param pkt source packet
  * \param param target symbol
  */
void pkt_begin(packet_t* pkt, sym_t* sym);

/** Return packet end.
  * Param length + 1.
  */
void* pkt_end(packet_t* pkt);

/** Dump packet (debugging).
  */
void pkt_dump(const char* pkt, uint32_t size);

/** Next symbol.
  * \return current symbol ptr
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

/** Pack size to byte array.
  * \warning Array has to be at least 5B long for uint32.
  * \return packed size, -1 on error
  */
int pack_size(uint32_t val, char* dst);

/** Unpack size from byte array.
  * \warning Array has to be at least 5B long for uint32.
  * \return unpacked value
  */
int unpack_size(const char* src, uint32_t* dst);

#ifdef __cplusplus
}
#endif

#endif // __protocol_h__
