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

/** ASN.1 semantic types
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
   CallType       = StructureType|SequenceType,

   // Calls
   NullRequest    = CallType,       // Ping
   UsbInit        = CallType  +  1, // usb_init()
   UsbFindBusses  = CallType  +  2, // usb_find_busses()
   UsbFindDevices = CallType  +  3, // usb_find_devices()
   UsbGetBusses   = CallType  +  4, // usb_bus* usb_get_busses()

} Type;

/* Packet manipulation. */
#ifdef __cplusplus
extern "C"
{
#endif

/** Packet structure. */
typedef struct {
   unsigned size;
   unsigned pos;
   char* buf;
} packet_t;

/** Parameter structure. */
typedef char* param_t;

#define PACKET_MINSIZE 3 // 1B op + 2B size
#define PARAM_TLEN     sizeof(char)
#define PARAM_LLEN     sizeof(uint8_t)
#define PARAM_LEN      PARAM_TLEN+PARAM_LLEN

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
packet_t* pkt_new(int size);

/** Free allocated packet.
  * Use only with dynamically allocated packets.
  * \param pkt freed packet
  */
void pkt_del(packet_t* pkt);

/** Initialize packet.
  * \param pkt initialized packet
  * \param op packet opcode
  */
void pkt_init(packet_t* pkt, Type op);

/** Return packet size.
  * \param pkt packet
  */
int pkt_size(packet_t* pkt);

/** Append parameter to packet.
  * \param pkt packet
  * \param type parameter type
  * \param len  parameter size
  * \param val  parameter value
  * \return bytes written
  */
int pkt_append(packet_t* pkt, Type type, uint16_t len, void* val);

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
  * \return recv() return value
  */
int pkt_recv(int fd, packet_t* dst);

/** Receive packet header.
  * Ensure buf is at least PACKET_MINSIZE.
  */
int pkt_recv_header(int fd, char* buf);

/** Receive packet payload.
  * Ensure buf contains header.
  */
int pkt_recv_payload(int fd, char* buf);

/** Return packet first symbol.
  * \param pkt source packet
  * \param param target symbol
  */
void pkt_begin(packet_t* pkt, param_t* param);

/** Return packet end.
  * Param length + 1.
  */
void* pkt_end(packet_t* pkt);

/** Dump packet (debugging). */
void pkt_dump(const char* pkt, int size);

/** Return parameter type.
  * \return parameter type
  */
#define param_type(param) (char)*(param)

/** Return parameter length.
  * \return parameter length
  */
#define param_len(param) (uint16_t)*((param) + PARAM_TLEN)

/** Return parameter value.
  * \return parameter value
  */
#define param_val(param, type) *(type*)((param) + PARAM_LEN)

/** Next parameter.
  */
#define param_next(param) ((param) + PARAM_LEN + param_len((param)))

#ifdef __cplusplus
}
#endif

#endif // __protocol_h__
