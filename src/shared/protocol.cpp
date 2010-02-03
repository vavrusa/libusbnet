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

#include "protocol.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>

Block::Block(buffer_t& sharedbuf, int pos)
   : mBuf(sharedbuf), mPos(pos), mCursor(0), mSize(0)
{
   // Seek end pos
   if(mPos < 0)
      mPos = mBuf.size();

   mCursor = mPos;
}

Block& Block::append(const char* str, size_t size) {
   if(size == 0)
      size = strlen(str);

   mBuf.insert(mCursor, str, size);
   mSize += size;
   mCursor += size;
   return *this;
}

Block& Block::addNumeric(Type type, uint8_t len, uint32_t val)
{
   // Check
   if(len != sizeof(uint32_t) &&
      len != sizeof(uint16_t) &&
      len != sizeof(uint8_t))
      return *this;

   // Push Type and Length
   push((uint8_t) type);
   push(len);

   // Cast to ensure correct data on both Big and Little-Endian hosts
   if(len == sizeof(uint32_t)) {
      append((const char*) &val, sizeof(uint32_t));
   }
   if(len == sizeof(uint16_t)) {
      uint16_t vval = val;
      append((const char*) &vval, sizeof(uint16_t));
   }
   if(len == sizeof(uint8_t)) {
      uint8_t vval = val;
      append((const char*) &vval, sizeof(uint8_t));
   }

   return *this;
}


Block& Block::addString(const char* str, Type type)
{
   if(str != 0) {
      int len = strlen(str);
      if(len > 255) {
         std::cerr << "Warning: strings of more than 255 chars not supported yet.\n";
      }
      push((uint8_t) type);
      push((uint8_t) len);
      append(str);
   }

   return *this;
}

Block& Block::finalize()
{
   // Remaining bufsize
   uint16_t ssize = mBuf.size() - startPos() - 1;

   // Main struct
   mBuf.insert(startPos() + 1, (const char*) &ssize, sizeof(ssize));

   return *this;
}

int Packet::recv(int fd)
{
   // Prepare buffer
   uint16_t size = PACKET_MINSIZE;
   mBuf.resize(size);
   if(pkt_recv_header(fd, (char*) mBuf.data()) < 0)
      return -1;

   // Reserve buffer for payload
   size = (uint16_t) *(mBuf.data() + 1);
   mBuf.resize(PACKET_MINSIZE + size);
   if(pkt_recv_payload(fd, (char*) mBuf.data()) < 0)
      return -1;

   return size + PACKET_MINSIZE;
}

void Packet::dump() {
   pkt_dump(mBuf.data(), size());
}

int Packet::send(int fd) {
   finalize();
   return pkt_send(fd, mBuf.data(), size());
}
