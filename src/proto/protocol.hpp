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

/** \page protopp_page
    <h2>Protocol C++ API</h2>
    Each entity is represented as Type-Length-Value.
    Example:
    \code
       packet = 1 {     // 0x31 - Type, 0x06 - Length, is structural
         int(4B) 2;     // 0x02 - Type, 0x04 - Length, 0x02 0x00 0x00 0x00 - Value
       }
    \endcode
    <h3>How to parse packet</h3>
    \code
      Iterator it(pkt); // Create iterator from packet reference.
      printf("int(4B): %d\n", it.getInt()); // Interpret item as integer.
    \endcode
    <h3>How to write packet</h3>
    \code
      Packet pkt(UsbInit); // Create packet.
      pkt.addInt32(someval); // Append 32bit integer.
      pkt.send(fd); // Send packet.
    \endcode
  */

/** C++ protocol abstraction. */
namespace Proto
{

/* Types */
typedef std::string ByteBuffer;

/** Class contains data in given BER structure (block).
    Suitable for reading and writing blocks, TLV attributes, raw values.
  */
class Struct
{
   public:
   Struct(ByteBuffer& sharedbuf, int pos = -1);

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
   Struct& push(char ch) {
      mBuf.insert(mCursor, 1, (char) ch);
      ++mSize; ++mCursor;
      return *this;
   }

   /** Write encoded length. */
   Struct& pushPacked(uint32_t val);

   /** Append raw data. */
   Struct& append(const char* str, size_t size = 0);

   /** Finalize block, insert block size in header. */
   Struct& finalize();

   /** Begin new block. */
   Struct writeBlock(uint8_t type = InvalidType) {
      Struct block(mBuf, mBuf.size());
      if(type != InvalidType)
         block.push(type);
      return block;
   }

   /** Add encoded numeric value.
     * Converts integer byte-order.
     */
   Struct& addNumeric(uint8_t type, uint8_t len, uint32_t val = 0);

   /** Add octet string. */
   Struct& addString(const char* str, uint8_t type = OctetType);

   /** Add boolean. */
   Struct& addBool(bool val) {
      return addNumeric(BoolType, 1, (uint8_t) val);
   }

   /** Add raw data. */
   Struct& addData(const char* data, size_t size, uint8_t type = RawType);

   /** Append 8bit long unsigned integer. */
   Struct& addUInt8(uint8_t val) {
      return addNumeric(UnsignedType, 1, val);
   }

   /** Append 16bit long unsigned integer. */
   Struct& addUInt16(uint16_t val) {
      return addNumeric(UnsignedType, 2, val);
   }

   /** Append 32bit long unsigned integer. */
   Struct& addUInt32(uint32_t val) {
      return addNumeric(UnsignedType, 4, val);
   }

   /** Append 8bit long signed integer. */
   Struct& addInt8(int8_t val) {
      return addNumeric(IntegerType, 1, val);
   }

   /** Append 16bit long signed integer. */
   Struct& addInt16(int16_t val) {
      return addNumeric(IntegerType, 2, val);
   }

   /** Append 32bit long signed integer. */
   Struct& addInt32(int32_t val) {
      return addNumeric(IntegerType, 4, val);
   }

   protected:
   void setbuf(ByteBuffer& buf) {
      mBuf = buf;
   }

   private:
      ByteBuffer& mBuf;
      int mPos, mCursor, mSize;
};


/** Class represents single value in BER encoding.
    Contains information about Type, Length and Value with
    conversion methods.
  */
class Iterator
{
   public:
      Iterator(Struct& block)
         : mType(InvalidType), mLength(0), mValue(0), mBlock(block), mPos(0)
      {
         enter();
      }

      /** Next value.
        */
      bool next();

      /** Enter structural value.
        */
      bool enter();

      /** Return symbol type.
        */
      uint8_t type() { return mType; }

      /** Return value length.
        */
      uint32_t length() { return mLength; }

      /** Return value as unsigned integer, depending on length.
        */
      unsigned getUInt()   {
         switch(mLength) {
            case sizeof(uint8_t): return getUInt8(); break;
            case sizeof(uint16_t): return getUInt16(); break;
            case sizeof(uint32_t): default: return getUInt32(); break;
         }
         return 0;
      }

      /** Return value as integer, depending on length.
        */
      int getInt()   {
         switch(mLength) {
         case sizeof(int8_t): return getInt8(); break;
         case sizeof(int16_t): return getInt16(); break;
         case sizeof(int32_t): default: return getInt32(); break;
         }
         return 0;
      }

      /** Return value as 8bit unsigned int.
        */
      uint8_t  getUInt8() { return *((uint8_t*) getVal()); }

      /** Return value as 16bit unsigned int.
        */
      uint16_t getUInt16() { return ntohs(*((uint16_t*) getVal())); }

      /** Return value as 32bit unsigned int.
        */
      uint32_t getUInt32() { return ntohl(*((uint32_t*) getVal())); }

      /** Return value as 8bit int.
        */
      int8_t  getInt8() { return *((int8_t*) getVal()); }

      /** Return value as 16bit int.
        */
      int16_t getInt16() { return ntohs(*((int16_t*) getVal())); }

      /** Return value as 32bit int.
        */
      int32_t getInt32() { return ntohl(*((int32_t*) getVal())); }

      /** Return value as byte array.
        */
      const char* getByteArray() { return getVal(); }

   protected:
      uint8_t setType(uint8_t val) { return mType = val; }
      uint32_t setLength(uint32_t val) { return mLength = val; }
      void setValue(const char* val) { mValue = val; }

      /** Return current value and move to next.
        */
      const char* getVal() {
        const char* ret = mValue;
        next();
        return ret;
      }

   private:
      uint8_t  mType;
      uint32_t mLength;
      const char* mValue;
      Struct& mBlock;
      int mPos;
};


/** Packet C++ abstraction.
  */
class Packet : public Struct
{
   public:

   /** Create on new/existing buffer. */
   Packet(uint8_t op = InvalidType)
      : Struct(mBuf, 0) {
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

}

#endif // __protocol_hpp__
/** @} */
