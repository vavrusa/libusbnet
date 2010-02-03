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

#ifndef __protocol_hpp__
#define __protocol_hpp__
#include "protocol.h"
#include <string>
#include <vector>

/* Types */
typedef std::string buffer_t;

/** Class contains data in given BER structure (block).
    Suitable for reading and writing blocks, TLV attributes, raw values.
  */
class Block
{
   public:
   Block(buffer_t& sharedbuf, int pos = -1);

   /** Return block size. */
   size_t size() {
      return mSize;
   }

   /** Return block starting position. */
   int startPos() {
      return mPos;
   }

   /** Return block cursor position. */
   int currentPos() {
      return mCursor;
   }

   /** Push raw byte. */
   Block& push(char ch) {
      mBuf.insert(mCursor, 1, (char) ch);
      ++mSize; ++mCursor;
      return *this;
   }

   /** Append raw data. */
   Block& append(const char* str, size_t size = 0);

   /** Finalize block, insert block size in header. */
   Block& finalize();

   /** Begin new block. */
   Block writeBlock(Type type = InvalidType) {
      Block block(mBuf, mBuf.size());
      if(type != InvalidType)
         block.push(type);
      return block;
   }

   /** Add encoded numeric value. */
   Block& addNumeric(Type type, uint8_t len, uint32_t val = 0);

   /** Add octet string. */
   Block& addString(const char* str, Type type = OctetType);

   /** Add boolean. */
   Block& addBool(bool val) {
      return addNumeric(BoolType, 1, (uint8_t) val);
   }

   /* Integer encoding - 8,16,32 bits */
   Block& addUInt8(uint8_t val) {
      return addNumeric(IntegerType, 1, val);
   }
   Block& addUInt16(uint16_t val) {
      return addNumeric(IntegerType, 2, val);
   }
   Block& addUInt32(uint32_t val) {
      return addNumeric(IntegerType, 4, val);
   }

   protected:
   void setbuf(buffer_t& buf) {
      mBuf = buf;
   }

   private:
      buffer_t& mBuf;
      int mPos, mCursor, mSize;
};


/** Packet C++ abstraction.
  */
class Packet : public Block
{
   public:

   /** Create on new/existing buffer. */
   Packet(Type op = InvalidType)
      : Block(mBuf, 0) {
      if(op != InvalidType) {
         push(op);
      }
   }

   /** Return packet opcode. */
   uint8_t op() {
      return mBuf.at(0);
   }

   /** Clear buffered data. */
   void clear() {
      mBuf.clear();
   }

   /** Returns total packet size. */
   size_t size() {
      return mBuf.size();
   }

   /** Returns data as raw char array. */
   const char* data() {
      return mBuf.data();
   }

   /** Hex-dump current data (debugging). */
   void dump();

   /** Receive packet from socket. */
   int recv(int fd);

   /** Send packet to socket. */
   int send(int fd);

   private:
   std::string mBuf;
};

#endif // __protocol_hpp__
