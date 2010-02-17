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
/*! \file protocol.hpp
    \brief Protocol abstraction and packet handling in C++.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup protocpp
    \ingroup proto
    @{
  */
#ifndef __protocol_hpp__
#define __protocol_hpp__
#include "protobase.h"
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
   virtual size_t size() {
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

   /** Return block data. */
   const char* data() {
      return mBuf.data() + mPos;
   }

   /** Push raw byte. */
   Block& push(char ch) {
      mBuf.insert(mCursor, 1, (char) ch);
      ++mSize; ++mCursor;
      return *this;
   }

   /** Write encoded length. */
   Block& pushPacked(uint32_t val);

   /** Append raw data. */
   Block& append(const char* str, size_t size = 0);

   /** Finalize block, insert block size in header. */
   Block& finalize();

   /** Begin new block. */
   Block writeBlock(uint8_t type = InvalidType) {
      Block block(mBuf, mBuf.size());
      if(type != InvalidType)
         block.push(type);
      return block;
   }

   /** Add encoded numeric value. */
   Block& addNumeric(uint8_t type, uint8_t len, uint32_t val = 0);

   /** Add octet string. */
   Block& addString(const char* str, uint8_t type = OctetType);

   /** Add boolean. */
   Block& addBool(bool val) {
      return addNumeric(BoolType, 1, (uint8_t) val);
   }

   /** Add raw data. */
   Block& addData(const char* data, size_t size, uint8_t type = RawType);

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
   Block& addInt8(int8_t val) {
      return addNumeric(IntegerType, 1, val);
   }
   Block& addInt16(int16_t val) {
      return addNumeric(IntegerType, 2, val);
   }
   Block& addInt32(int32_t val) {
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


/** Class represents single value in BER encoding.
    Contains information about Type, Length and Value with
    conversion methods.
  */
class Symbol
{
   public:
      Symbol(Block& block)
         : mType(InvalidType), mLength(0), mValue(0), mBlock(block), mPos(0)
      {
         enter();
      }

      // Next
      bool next();

      // Enter
      bool enter();

      // Return symbol type
      uint8_t type() { return mType; }

      // Return symbol length
      uint32_t length() { return mLength; }

      // Return value as unsigned integer, depending on length
      unsigned asUInt()   {
         switch(mLength) {
            case 1: return asUInt8(); break;
            case 2: return asUInt16(); break;
            case 4: default: return asUInt32(); break;
         }
         return 0;
      }

      // Return value as integer, depending on length
      int asInt()   {
         switch(mLength) {
            case 1: return asInt8(); break;
            case 2: return asInt16(); break;
            case 4: default: return asInt32(); break;
         }
         return 0;
      }

      // Return value as 8bit unsigned int
      uint8_t  asUInt8() { return (uint8_t) mValue[0]; }

      // Return value as 16bit unsigned int
      uint16_t asUInt16() { return *((uint16_t*) mValue); }

      // Return value as 32bit unsigned int
      uint32_t asUInt32() { return *((uint32_t*) mValue); }

      // Return value as 8bit int
      int8_t  asInt8() { return (int8_t) mValue[0]; }

      // Return value as 16bit int
      int16_t asInt16() { return *((int16_t*) mValue); }

      // Return value as 32bit int
      int32_t asInt32() { return *((int32_t*) mValue); }

      // Return value as string
      const char* asString() { return mValue; }

   protected:
      uint8_t setType(uint8_t val) { return mType = val; }
      uint32_t setLength(uint32_t val) { return mLength = val; }
      void setValue(const char* val) { mValue = val; }

   private:
      uint8_t  mType;
      uint32_t mLength;
      const char* mValue;
      Block& mBlock;
      int mPos;
};


/** Packet C++ abstraction.
  */
class Packet : public Block
{
   public:

   /** Create on new/existing buffer. */
   Packet(uint8_t op = InvalidType)
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
/** @} */
