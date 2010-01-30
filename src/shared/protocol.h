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

/** ASN.1/BER semantic types
  */
typedef enum {

   // Basic types
   BoolType       = 0x01,
   IntegerType    = 0x02,
   NullType       = 0x05,
   OctetType      = 0x04,
   SequenceType   = 0x10,
   EnumType       = 0x0A,
   SetType        = 0x11,
   CallType       = 0x20,

   // Calls
   NullRequest    = CallType,       // Ping
   UsbInit        = CallType  +  1, // usb_init()
} Type;

/* Packet manipulation. */
#ifdef __cplusplus
extern "C"
{
#endif

char* pkt_init(char* buf, Type t);

#ifdef __cplusplus
}
#endif

#endif // __protocol_h__
