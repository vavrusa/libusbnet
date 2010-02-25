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
/*! \file usbservice.cpp
    \brief Server handler implementation for libusb.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup server
    @{
  */
#include "usbservice.hpp"
#include "protocol.hpp"
#include <netinet/tcp.h>

UsbService::UsbService(int fd)
   : ServerSocket(fd)
{
   // Disable TCP buffering
   int flag = 1;
   setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

UsbService::~UsbService()
{
   // Close open devices
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      log_msg("UsbService: closing open device %p", *i);
      ::usb_close(*i);
   }
   mOpenList.clear();
}

bool UsbService::handle(int fd, Packet& pkt)
{
   // Check size
   if(pkt.size() <= 0)
      return false;

   // Packet handling
   switch(pkt.op())
   {
      case UsbInit:        usb_init(fd, pkt);         break;
      case UsbFindBusses:  usb_find_busses(fd, pkt);  break;
      case UsbFindDevices: usb_find_devices(fd, pkt); break;
      case UsbOpen:        usb_open(fd, pkt);         break;
      case UsbClose:       usb_close(fd, pkt);        break;
      case UsbControlMsg:  usb_control_msg(fd, pkt);  break;
      case UsbClaimInterface: usb_claim_interface(fd, pkt); break;
      case UsbReleaseInterface: usb_release_interface(fd, pkt); break;
      case UsbDetachKernelDriver: usb_detach_kernel_driver(fd, pkt); break;
      case UsbBulkRead:    usb_bulk_read(fd, pkt);    break;
      case UsbBulkWrite:   usb_bulk_write(fd, pkt);   break;
      case UsbSetConfiguration: usb_set_configuration(fd, pkt); break;
      case UsbSetAltInterface: usb_set_altinterface(fd, pkt); break;
      case UsbResetEp:     usb_resetep(fd, pkt); break;
      case UsbClearHalt:   usb_clear_halt(fd, pkt); break;
      case UsbReset:       usb_reset(fd, pkt); break;
      case UsbInterruptWrite: usb_interrupt_write(fd, pkt); break;
      case UsbInterruptRead: usb_interrupt_read(fd, pkt); break;
      default:
         log_msg("%s: unhandled call type: 0x%02x (socket fd %d)", __func__, pkt.op(), fd);
         return false;
         break;
   }

   return true;
}

void UsbService::usb_init(int fd, Packet& in)
{
   // Call, no ACK
   debug_msg("called");
   ::usb_init();
}

void UsbService::usb_find_busses(int fd, Packet& in)
{
   // Call
   // Can't guarantee correct number in case of multi-client environment
   int res = ::usb_find_busses();
   debug_msg("returned %d", res);

   // Send result
   Packet pkt(UsbFindBusses);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_find_devices(int fd, Packet& in)
{
   // Can't guarantee correct result in case of multi-client environment,
   // but anything >=0 should be fine.
   int res = ::usb_find_devices();
   debug_msg("returned %d", res);

   // Prepare result packet
   Packet pkt(UsbFindDevices);
   pkt.addInt32(res);

   // Add existing busses and devices
   struct usb_bus* bus = 0;
   for(bus = ::usb_get_busses(); bus; bus = bus->next) {

      Struct block = pkt.writeBlock(StructureType);
      block.addString(bus->dirname);
      block.addUInt32(bus->location);

      //! \todo Implement device children ptrs.
      for(struct usb_device* dev = bus->devices; dev; dev = dev->next) {

         Struct devBlock = block.writeBlock(SequenceType);
         devBlock.addString(dev->filename);
         devBlock.addUInt8(dev->devnum);

         // Copy device descriptor
         // Convert byte-order for 16/32bit values
         struct usb_device_descriptor desc_m;
         memcpy(&desc_m, &dev->descriptor, sizeof(struct usb_device_descriptor));
         desc_m.bcdUSB = htons(desc_m.bcdUSB);
         desc_m.idVendor = htons(desc_m.idVendor);
         desc_m.idProduct = htons(desc_m.idProduct);
         desc_m.bcdDevice = htons(desc_m.bcdDevice);
         devBlock.addData((const char*) &desc_m, sizeof(struct usb_device_descriptor));

         // Add configurations
         for(unsigned c = 0; c < dev->descriptor.bNumConfigurations; ++c) {
            struct usb_config_descriptor* cfg = &dev->config[c];

            // Copy config descriptor
            struct usb_config_descriptor cfg_m;
            memcpy(&cfg_m, cfg, sizeof(struct usb_config_descriptor));
            cfg_m.wTotalLength = htons(cfg_m.wTotalLength);
            devBlock.addData((const char*) &cfg_m, sizeof(struct usb_config_descriptor));

            // Add interfaces
            for(unsigned i = 0; i < cfg->bNumInterfaces; ++i) {
               struct usb_interface* iface = &cfg->interface[i];
               devBlock.addInt32(iface->num_altsetting);

               // Add interface settings
               for(unsigned j = 0; j < iface->num_altsetting; ++j) {
                  struct usb_interface_descriptor* altsetting = &iface->altsetting[j];

                  // Copy altsetting - no conversions apply
                  devBlock.addData((const char*) altsetting, sizeof(struct usb_interface_descriptor));

                  // Add endpoints
                  for(unsigned k = 0; k < altsetting->bNumEndpoints; ++k) {
                     struct usb_endpoint_descriptor* endpoint = &altsetting->endpoint[k];

                     // Copy endpoint
                     struct usb_endpoint_descriptor endpoint_m;
                     memcpy(&endpoint_m, endpoint, sizeof(struct usb_endpoint_descriptor));
                     endpoint_m.wMaxPacketSize = htons(endpoint_m.wMaxPacketSize);
                     devBlock.addData((const char*) &endpoint_m, sizeof(struct usb_endpoint_descriptor));
                  }

                  // Add extra interface descriptors
                  if(altsetting->extralen > 0){
                     devBlock.addInt32(altsetting->extralen);
                     devBlock.addData((const char*)altsetting->extra, altsetting->extralen);
                  }
                  else
                     devBlock.addInt32(0);
               }
            }
         }

         devBlock.finalize();
      }

      // Finalize block
      block.finalize();
   }

   // Send result
   pkt.send(fd);
}

void UsbService::usb_open(int fd, Packet& in)
{
   Iterator it(in);
   unsigned busid = it.getUInt();
   unsigned devid = it.getUInt();

   // Find device
   struct usb_device* rdev = NULL;
   for(struct usb_bus* bus = ::usb_get_busses(); bus; bus = bus->next) {

      // Find bus
      if(bus->location == busid) {

         // Find device
         for(struct usb_device* dev = bus->devices; dev; dev = dev->next) {

            // Device match
            if(dev->devnum == devid) {
               rdev = dev;
               break;
            }
         }

         break;
      }
   }

   // Open device
   int res = -1;
   int openfd = -1;
   usb_dev_handle* udev = NULL;
   if(rdev != NULL) {
      // Check successful open
      if((udev = ::usb_open(rdev)) != NULL) {
         mOpenList.push_back(udev);
         res = 0;
         openfd = udev->fd;
      }
   }


   debug_msg("bus_id %u, dev_id %u = %d (fd %d)", busid, devid, res, openfd);

   // Return result
   Packet pkt(UsbOpen);
   pkt.addInt8(res);
   pkt.addInt32(openfd);
   pkt.send(fd);
}

void UsbService::usb_close(int fd, Packet& in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         mOpenList.erase(i);
         res = ::usb_close(h);
         break;
      }
   }

   debug_msg("fd %d = %d", devfd, res);

   // Return result
   Packet pkt(UsbClose);
   pkt.addInt8(res);
   pkt.send(fd);
}

void UsbService::usb_set_configuration(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   int configuration = it.getInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_set_configuration(h, configuration);
         configuration = h->config;
         break;
      }
   }

   debug_msg("fd %d, configuration %d = %d", devfd, configuration, res);

   // Return result
   Packet pkt(UsbSetConfiguration);
   pkt.addInt32(res);
   pkt.addInt32(configuration);
   pkt.send(fd);
}

void UsbService::usb_set_altinterface(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   int alternate = it.getInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_set_altinterface(h, alternate);
         alternate = h->altsetting;
         break;
      }
   }

   debug_msg("fd %d, alternate %d = %d", devfd, alternate, res);

   // Return result
   Packet pkt(UsbSetAltInterface);
   pkt.addInt32(res);
   pkt.addInt32(alternate);
   pkt.send(fd);
}

void UsbService::usb_resetep(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   unsigned int ep = it.getUInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_resetep(h, ep);
         break;
      }
   }

   debug_msg("fd %d, ep %d = %d", devfd, ep, res);

   // Return result
   Packet pkt(UsbResetEp);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_clear_halt(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   unsigned int ep = it.getUInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_clear_halt(h, ep);
         break;
      }
   }

   debug_msg("fd %d, ep %d = %d", devfd, ep, res);

   // Return result
   Packet pkt(UsbClearHalt);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_reset(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_reset(h);
         break;
      }
   }

   debug_msg("fd %d = %d", devfd, res);

   // Return result
   Packet pkt(UsbReset);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_claim_interface(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   int index = it.getInt();
   int res = -1;

   // Find open device
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_claim_interface(h, index);
         break;
      }
   }

   debug_msg("fd %d = %d", devfd, res);

   // Return result
   Packet pkt(UsbClaimInterface);
   pkt.addInt32((int32_t) res);
   pkt.send(fd);
}

void UsbService::usb_release_interface(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   int index = it.getInt();
   int res = -1;

   // Find open device
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
         res = ::usb_release_interface(h, index);
         res = 0;
         break;
      }
   }

   debug_msg("fd %d = %d", devfd, res);

   // Return result
   Packet pkt(UsbReleaseInterface);
   pkt.addInt32((int32_t) res);
   pkt.send(fd);
}

void UsbService::usb_detach_kernel_driver(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();
   int index = it.getInt();

   // Find open device
   int res = -1;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      usb_dev_handle* h = *i;
      if(h->fd == devfd) {
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
         res = ::usb_detach_kernel_driver_np(h, index);
#else
         res = 0;
#endif
         break;
      }
   }

   debug_msg("fd %d, index %d = %d", devfd, index, res);

   // Return result
   Packet pkt(UsbDetachKernelDriver);
   pkt.addInt32((int32_t) res);
   pkt.send(fd);
}

void UsbService::usb_control_msg(int fd, Packet& in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   usb_dev_handle* h = NULL;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      if((*i)->fd == devfd) {
         h = *i;
         break;
      }
   }

   // Device not found
   int res = -1;
   char* data = NULL;
   if(h != NULL) {

      // Call function
      int reqtype = it.getInt();
      int request = it.getInt();
      int value   = it.getInt();
      int index   = it.getInt();
      int size    = it.length();
      data  = (char*) it.getByteArray();
      int timeout = it.getInt();

      res = ::usb_control_msg(h, reqtype, request, value, index, data, size, timeout);
      debug_msg("fd %d = %d", devfd, res);
   }

   // Return packet
   Packet pkt(UsbControlMsg);
   pkt.addInt32(res);
   pkt.addData(data, (res < 0) ? 0 : res, OctetType);
   pkt.send(fd);
}

void UsbService::usb_bulk_read(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   usb_dev_handle* h = NULL;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      if((*i)->fd == devfd) {
         h = *i;
         break;
      }
   }

   // Device not found
   int res = -1;
   char* data = NULL;
   int ep = it.getInt();
   int size = it.getInt();
   int timeout = it.getInt();
   if(h != NULL && size > 0) {

      // Call function
      data = new char[size];
      res = ::usb_bulk_read(h, ep, data, size, timeout);
      debug_msg("fd %d = %d", devfd, res);
   }

   // Return packet
   Packet pkt(UsbBulkRead);
   pkt.addInt32(res);
   pkt.addData(data, (res < 0) ? 0 : res, OctetType);
   pkt.send(fd);

   // Free data
   if(data != NULL)
      delete data;
}

void UsbService::usb_bulk_write(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   usb_dev_handle* h = NULL;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      if((*i)->fd == devfd) {
         h = *i;
         break;
      }
   }

   // Device not found
   int res = -1;
   int ep = it.getInt();
   int size = it.length();
   char* data = (char*) it.getByteArray();
   int timeout = it.getInt();
   if(h != NULL && size > 0) {

      // Call function
      res = ::usb_bulk_write(h, ep, data, size, timeout);
      debug_msg("fd %d = %d", devfd, res);
   }

   // Return packet
   Packet pkt(UsbBulkWrite);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_interrupt_write(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   usb_dev_handle* h = NULL;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      if((*i)->fd == devfd) {
         h = *i;
         break;
      }
   }

   // Device not found
   int res = -1;
   int ep = it.getInt();
   int size = it.length();
   char* data = (char*) it.getByteArray();
   int timeout = it.getInt();
   if(h != NULL && size > 0) {

      // Call function
      res = ::usb_interrupt_write(h, ep, data, size, timeout);
      debug_msg("fd %d = %d", devfd, res);
   }

   // Return packet
   Packet pkt(UsbInterruptWrite);
   pkt.addInt32(res);
   pkt.send(fd);
}

void UsbService::usb_interrupt_read(int fd, Packet &in)
{
   Iterator it(in);
   int devfd = it.getInt();

   // Find open device
   usb_dev_handle* h = NULL;
   std::list<usb_dev_handle*>::iterator i;
   for(i = mOpenList.begin(); i != mOpenList.end(); ++i) {
      if((*i)->fd == devfd) {
         h = *i;
         break;
      }
   }

   // Device not found
   int res = -1;
   char* data = NULL;
   int ep = it.getInt();
   int size = it.getInt();
   int timeout = it.getInt();
   if(h != NULL && size > 0) {

      // Call function
      data = new char[size];
      res = ::usb_interrupt_read(h, ep, data, size, timeout);
      debug_msg("fd %d = %d", devfd, res);
   }

   // Return packet
   Packet pkt(UsbInterruptRead);
   pkt.addInt32(res);
   pkt.addData(data, (res < 0) ? 0 : res, OctetType);
   pkt.send(fd);

   // Free data
   if(data != NULL)
      delete data;
}
/** @} */
