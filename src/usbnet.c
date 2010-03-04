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
/*! \file usbnet.c
    \brief Reimplementation of libusb prototypes.
    \author Marek Vavrusa <marek@vavrusa.com>
    \addtogroup libusbnet
    @{
  */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "usbnet.h"
#include "protocol.h"

#ifdef USE_USB_CONST_BUFFERS
typedef const char *usb_buf_t;
#else
typedef char *usb_buf_t;
#endif

/** Imported from libusb-0.1/descriptors.c
 */
static void usb_destroy_configuration(struct usb_device *dev);

//! Remote socket filedescriptor
static int __remote_fd = -1;

//! Remote USB busses with devices
static struct usb_bus* __orig_bus   = NULL;
static struct usb_bus* __remote_bus = NULL;
extern struct usb_bus* usb_busses;

void session_teardown() {

   // Unhook global variable
   debug_msg("unhooking virtual bus ...");
   usb_busses = __orig_bus;

   // Free global packet
   debug_msg("deallocating shared packet ...");
   if(pkt_shared() != NULL)
      pkt_free(pkt_shared());

   // Free busses
   debug_msg("freeing busses ...");
   struct usb_bus* cur = NULL;
   while(__remote_bus != NULL) {

      // Shift bus ptr
      cur = __remote_bus;
      __remote_bus = cur->next;

      // Free bus devices
      struct usb_device* dev = NULL;
      while(cur->devices != NULL) {
         dev = cur->devices;
         cur->devices = dev->next;

         // Destroy configuration and free device
         usb_destroy_configuration(dev);
         if(dev->children != NULL)
            free(dev->children);
         free(dev);
      }

      // Free bus
      free(cur);
   }
}

int session_get() {

   // Check exit function
   static char exitf_hooked = 0;
   if(!exitf_hooked) {
      atexit(&session_teardown);
      exitf_hooked = 1;
   }

   // Retrieve remote sock from SHM
   if(__remote_fd == -1) {
      __remote_fd = ipc_get_remote();
   }

   if(__remote_fd == -1) {
      fprintf(stderr, "IPC: unable to access remote fd\n");
      exit(1);
   }

   return __remote_fd;
}


/* libusb functions reimplementation.
 * \see http://libusb.sourceforge.net/doc/functions.html
 */

/* libusb(1):
 * Core functions.
 */

/** Initialize USB subsystem. */
void usb_init(void)
{
   // Initialize packet & remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Create buffer
   pkt_init(pkt, UsbInit);
   pkt_send(pkt, fd);
   pkt_release();

   // Initialize locally
   debug_msg("called");
}

/** Find busses on remote host.
    \warning Does not transfer busses to local virtual bus list.
  */
int usb_find_busses(void)
{
  // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Initialize pkt
   pkt_init(pkt, UsbFindBusses);
   pkt_send(pkt, fd);

   // Get number of changes
   int res = 0;
   Iterator it;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbFindBusses) {
      if(pkt_begin(pkt, &it) != NULL) {
         res = iter_getint(&it);
      }
   }

   // Return remote result
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/** Find devices on remote host.
  * Create new devices on local virtual bus.
  * \warning Function replaces global usb_busses variable from libusb.
  */
int usb_find_devices(void)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Create buffer
   pkt_init(pkt, UsbFindDevices);
   pkt_send(pkt, fd);

   // Get number of changes
   int res = 0;
   Iterator it;
   if(pkt_recv(fd, pkt) > 0) {
      pkt_begin(pkt, &it);

      // Get return value
      res = iter_getint(&it);

      // Allocate virtualbus
      struct usb_bus vbus;
      vbus.next = __remote_bus;
      struct usb_bus* rbus = &vbus;

      // Get busses
      while(!iter_end(&it)) {

         // Evaluate
         if(it.type == StructureType) {
            iter_enter(&it);

            // Allocate bus
            if(rbus->next == NULL) {

               // Allocate next item
               struct usb_bus* nbus = malloc(sizeof(struct usb_bus));
               memset(nbus, 0, sizeof(struct usb_bus));
               rbus->next = nbus;
               nbus->prev = rbus;
               rbus = nbus;
            }
            else
               rbus = rbus->next;

            // Read dirname
            strcpy(rbus->dirname, iter_getstr(&it));

            // Read location
            rbus->location = iter_getuint(&it);

            // Read devices
            struct usb_device vdev;
            vdev.next = rbus->devices;
            struct usb_device* dev = &vdev;
            while(it.type == SequenceType) {
               iter_enter(&it);

               // Initialize
               if(dev->next == NULL) {
                  dev->next = malloc(sizeof(struct usb_device));
                  memset(dev->next, 0, sizeof(struct usb_device));
                  dev->next->bus = rbus;
                  if(dev != &vdev)
                     dev->next->prev = dev;
                  if(rbus->devices == NULL)
                     rbus->devices = dev->next;
               }

               dev = dev->next;

               // Read filename
               strcpy(dev->filename, iter_getstr(&it));

               // Read devnum
               dev->devnum = iter_getuint(&it);

               // Read descriptor
               // Apply byte-order conversion for 16/32bit integers
               memcpy(&dev->descriptor, it.val, it.len);
               dev->descriptor.bcdUSB = ntohs(dev->descriptor.bcdUSB);
               dev->descriptor.idVendor = ntohs(dev->descriptor.idVendor);
               dev->descriptor.idProduct = ntohs(dev->descriptor.idProduct);
               dev->descriptor.bcdDevice = ntohs(dev->descriptor.bcdDevice);
               iter_next(&it);

               // Alloc configurations
               unsigned cfgid = 0, cfgnum = dev->descriptor.bNumConfigurations;
               dev->config = NULL;
               if(cfgnum > 0) {
                  dev->config = malloc(cfgnum * sizeof(struct usb_config_descriptor));
                  memset(dev->config, 0, cfgnum * sizeof(struct usb_config_descriptor));
               }

               // Read config
               while(it.type == RawType && cfgid < cfgnum) {
                  struct usb_config_descriptor* cfg = &dev->config[cfgid];
                  ++cfgid;

                  // Ensure struct under/overlap
                  int szlen = sizeof(struct usb_config_descriptor);
                  if(szlen > it.len)
                     szlen = it.len;

                  // Read config and apply byte-order conversion
                  memcpy(cfg, it.val, szlen);
                  cfg->wTotalLength = ntohs(cfg->wTotalLength);

                  // Allocate interfaces
                  cfg->interface = NULL;
                  if(cfg->bNumInterfaces > 0) {
                     cfg->interface = malloc(cfg->bNumInterfaces * sizeof(struct usb_interface));
                  }

                  //! \test Implement usb_device extra interfaces - are they needed?
                  cfg->extralen = 0;
                  cfg->extra = NULL;
                  iter_next(&it);

                  // Load interfaces
                  unsigned i, j, k;
                  for(i = 0; i < cfg->bNumInterfaces; ++i) {
                     struct usb_interface* iface = &cfg->interface[i];

                     // Read altsettings count
                     iface->num_altsetting = iter_getint(&it);

                     // Allocate altsettings
                     if(iface->num_altsetting > 0) {
                        iface->altsetting = malloc(iface->num_altsetting * sizeof(struct usb_interface_descriptor));
                     }

                     // Load altsettings
                     for(j = 0; j < iface->num_altsetting; ++j) {

                        // Ensure struct under/overlap
                        struct usb_interface_descriptor* as = &iface->altsetting[j];
                        int szlen = sizeof(struct usb_interface_descriptor);
                        if(szlen > it.len)
                           szlen = it.len;

                        // Read altsettings - no conversions apply
                        memcpy(as, it.val, szlen);
                        iter_next(&it);

                        // Allocate endpoints
                        as->endpoint = NULL;
                        if(as->bNumEndpoints > 0) {
                           size_t epsize = as->bNumEndpoints * sizeof(struct usb_endpoint_descriptor);
                           as->endpoint = malloc(epsize);
                           memset(as->endpoint, 0, epsize);
                        }

                        // Load endpoints
                        for(k = 0; k < as->bNumEndpoints; ++k) {
                           struct usb_endpoint_descriptor* endpoint = &as->endpoint[k];
                           int szlen = sizeof(struct usb_endpoint_descriptor);
                           if(szlen > it.len)
                              szlen = it.len;

                           // Read endpoint and apply conversion
                           memcpy(endpoint, it.val, szlen);
                           endpoint->wMaxPacketSize = ntohs(endpoint->wMaxPacketSize);
                           iter_next(&it);

                           // Null extra descriptors.
                           endpoint->extralen = 0;
                           endpoint->extra = NULL;
                        }

                        // Read extra interface descriptors
                        as->extralen = as_int(it.val, it.len);
                        iter_next(&it);

                        if(as->extralen > 0){
                            as->extra = malloc(as->extralen);

                            int szlen = as->extralen;
                            if(szlen > it.len)
                                szlen = it.len;

                            memcpy(as->extra, it.val, szlen);
                            iter_next(&it);
                        }
                        else
                            as->extra = NULL;
                     }
                  }
               }

               //log_msg("Bus %s Device %s: ID %04x:%04x", rbus->dirname, dev->filename, dev->descriptor.idVendor, dev->descriptor.idProduct);
            }

            // Free unused devices
            while(dev->next != NULL) {
               struct usb_device* ddev = dev->next;
               debug_msg("deleting device %03d", ddev->devnum);
               dev->next = ddev->next;
               free(ddev);
            }

         }
         else {
            debug_msg("unexpected item identifier 0x%02x", it.type);
            iter_next(&it);
         }
      }

      // Deallocate unnecessary busses
      while(rbus->next != NULL) {
         debug_msg("deleting bus %03d", rbus->next->location);
         struct usb_bus* bus = rbus->next;
         rbus->next = bus->next;
      }

      // Save busses
      if(__remote_bus == NULL) {
         __orig_bus = usb_busses;
         debug_msg("overriding global usb_busses from %p to %p", usb_busses, vbus.next);
      }

      __remote_bus = vbus.next;
      usb_busses = __remote_bus;
   }

   // Return remote result
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/** Return pointer to virtual bus list.
  */
struct usb_bus* usb_get_busses(void)
{
   //! \todo Merge both local - attached busses in future?
   debug_msg("returned %p", __remote_bus);
   return __remote_bus;
}

/* libusb(2):
 * Device operations.
 */

usb_dev_handle *usb_open(struct usb_device *dev)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbOpen);
   pkt_adduint(pkt, dev->bus->location);
   pkt_adduint(pkt, dev->devnum);
   pkt_send(pkt, fd);

   // Get response
   int res = -1, devfd = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbOpen) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
      devfd = iter_getint(&it);
   }

   // Evaluate
   usb_dev_handle* udev = NULL;
   if(res >= 0) {
      udev = malloc(sizeof(usb_dev_handle));
      udev->fd = devfd;
      udev->device = dev;
      udev->bus = dev->bus;
      udev->config = udev->interface = udev->altsetting = -1;
   }

   pkt_release();
   debug_msg("returned %d (fd %d)", res, devfd);
   return udev;
}

int usb_close(usb_dev_handle *dev)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbClose);
   pkt_addint(pkt, dev->fd);
   pkt_send(pkt, fd);

   // Free device
   free(dev);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbClose) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_set_configuration(usb_dev_handle *dev, int configuration)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbSetConfiguration);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, configuration);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbSetConfiguration) {
      Iterator it;
      pkt_begin(pkt, &it);

      // Read result
      res = iter_getint(&it);

      // Read callback configuration
      configuration = iter_getint(&it);
   }

   // Save configuration
   dev->config = configuration;

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_set_altinterface(usb_dev_handle *dev, int alternate)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbSetAltInterface);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, alternate);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbSetAltInterface) {
      Iterator it;
      pkt_begin(pkt, &it);

      // Read result
      res = iter_getint(&it);

      // Read callback configuration
      alternate = iter_getint(&it);
   }

   // Save configuration
   dev->altsetting = alternate;

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_resetep(usb_dev_handle *dev, unsigned int ep)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbResetEp);
   pkt_addint(pkt,  dev->fd);
   pkt_adduint(pkt, ep);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbResetEp) {
      Iterator it;
      pkt_begin(pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_clear_halt(usb_dev_handle *dev, unsigned int ep)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbClearHalt);
   pkt_addint(pkt, dev->fd);
   pkt_adduint(pkt, ep);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbClearHalt) {
      Iterator it;
      pkt_begin(pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_reset(usb_dev_handle *dev)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbReset);
   pkt_addint(pkt, dev->fd);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbReset) {
      Iterator it;
      pkt_begin(pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_claim_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbClaimInterface);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, interface);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbClaimInterface) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_release_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbReleaseInterface);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, interface);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbReleaseInterface) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(3):
 * Control transfers.
 */

int usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
        int value, int index, char *bytes, int size, int timeout)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbControlMsg);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, requesttype);
   pkt_addint(pkt, request);
   pkt_addint(pkt, value);
   pkt_addint(pkt, index);
   pkt_addstr(pkt, size, bytes);
   pkt_addint(pkt, timeout);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbControlMsg) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);

      if(res > 0) {
         int minlen = (res > size) ? size : res;
         memcpy(bytes, it.val, minlen);
      }

   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(4):
 * Bulk transfers.
 */

int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbBulkRead);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, ep);
   pkt_addint(pkt, size);
   pkt_addint(pkt, timeout);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbBulkRead) {

      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);

      if(res > 0) {
         int minlen = (res > size) ? size : res;
         memcpy(bytes, it.val, minlen);
      }

   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_bulk_write(usb_dev_handle *dev, int ep, usb_buf_t bytes, int size, int timeout)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbBulkWrite);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, ep);
   pkt_addstr(pkt, size,        bytes);
   pkt_addint(pkt, timeout);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbBulkWrite) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(5):
 * Interrupt transfers.
 */
int usb_interrupt_write(usb_dev_handle *dev, int ep, usb_buf_t bytes, int size, int timeout)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbInterruptWrite);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, ep);
   pkt_addstr(pkt, size, bytes);
   pkt_addint(pkt, timeout);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbInterruptWrite) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_interrupt_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Prepare packet
   pkt_init(pkt, UsbInterruptRead);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, ep);
   pkt_addint(pkt, size);
   pkt_addint(pkt, timeout);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbInterruptRead) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);

      if(res > 0) {
         int minlen = (res > size) ? size : res;
         memcpy(bytes, it.val, minlen);
      }

   }

   // Return response
   pkt_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(6):
 * Non-portable.
 */
#if LIBUSB_HAS_GET_DRIVER_NP

/* Imported from libusb-0.1.12/linux.h:38
 */
#define USB_MAXDRIVERNAME 255
struct usb_getdriver {
        unsigned int interface;
        char driver[USB_MAXDRIVERNAME + 1];
};

int usb_get_driver_np(usb_dev_handle *dev, int interface, char *name, unsigned int namelen)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbGetKernelDriver);
   pkt_addint(pkt,  dev->fd);
   pkt_addint(pkt,  interface);
   pkt_adduint(pkt, namelen);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbGetKernelDriver) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);

      // Error
      if(res) {
         error_msg("%s: could not get bound driver", __func__);
      }

      // Save string
      strncpy(name, iter_getstr(&it), namelen - 1);
      name[namelen - 1] = '\0';
   }

   pkt_release();
   debug_msg("returned %d (%s)", res, name);
   return res;
}
#endif

#if LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
int usb_detach_kernel_driver_np(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   Packet* pkt = pkt_claim();
   int fd = session_get();

   // Send packet
   pkt_init(pkt, UsbDetachKernelDriver);
   pkt_addint(pkt, dev->fd);
   pkt_addint(pkt, interface);
   pkt_send(pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt_op(pkt) == UsbDetachKernelDriver) {
      Iterator it;
      pkt_begin(pkt, &it);
      res = iter_getint(&it);
   }

   pkt_release();
   debug_msg("returned %d", res);
   return res;
}
#endif

/** \private
 * Imported from libusb-0.1.12 for forward compatibility with libusb-1.0.
 * This overrides libusb-0.1 as well as libusb-1.0 calls.
 * @see libusb-0.1.12/descriptors.c:23
 * @see libusb-0.1.12/usb.c:219
 * @see libusb-0.1.12/usb.c:230
 */
int usb_get_descriptor(usb_dev_handle *udev, unsigned char type,
        unsigned char index, void *buf, int size)
{
  memset(buf, 0, size);

  return usb_control_msg(udev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
                        (type << 8) + index, 0, buf, size, 1000);
}
int usb_get_string(usb_dev_handle *dev, int index, int langid, char *buf,
        size_t buflen)
{
  /** \private
   * We can't use usb_get_descriptor() because it's lacking the index
   * parameter. This will be fixed in libusb 1.0
   */
  return usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
                        (USB_DT_STRING << 8) + index, langid, buf, buflen, 1000);
}
int usb_get_string_simple(usb_dev_handle *dev, int index, char *buf, size_t buflen)
{
  char tbuf[255];	/* Some devices choke on size > 255 */
  int ret, langid, si, di;

  /*
   * Asking for the zero'th index is special - it returns a string
   * descriptor that contains all the language IDs supported by the
   * device. Typically there aren't many - often only one. The
   * language IDs are 16 bit numbers, and they start at the third byte
   * in the descriptor. See USB 2.0 specification, section 9.6.7, for
   * more information on this. */
  ret = usb_get_string(dev, 0, 0, tbuf, sizeof(tbuf));
  if (ret < 0)
    return ret;

  if (ret < 4)
    return -EIO;

  langid = tbuf[2] | (tbuf[3] << 8);

  ret = usb_get_string(dev, index, langid, tbuf, sizeof(tbuf));
  if (ret < 0)
    return ret;

  if (tbuf[1] != USB_DT_STRING)
    return -EIO;

  if (tbuf[0] > ret)
    return -EFBIG;

  for (di = 0, si = 2; si < tbuf[0]; si += 2) {
    if (di >= (buflen - 1))
      break;

    if (tbuf[si + 1])	/* high byte */
      buf[di++] = '?';
    else
      buf[di++] = tbuf[si];
  }

  buf[di] = 0;

  return di;
}

/** \private
  *  Imported from libusb-0.1.12 private interface as helper function.
  */
static void usb_destroy_configuration(struct usb_device *dev)
{
  int c, i, j, k;

  if (!dev->config)
    return;

  for (c = 0; c < dev->descriptor.bNumConfigurations; c++) {
    struct usb_config_descriptor *cf = &dev->config[c];

    if (!cf->interface)
      continue;

    for (i = 0; i < cf->bNumInterfaces; i++) {
      struct usb_interface *ifp = &cf->interface[i];

      if (!ifp->altsetting)
        continue;

      for (j = 0; j < ifp->num_altsetting; j++) {
        struct usb_interface_descriptor *as = &ifp->altsetting[j];

        if (as->extra)
          free(as->extra);

        if (!as->endpoint)
          continue;

        for (k = 0; k < as->bNumEndpoints; k++) {
          if (as->endpoint[k].extra)
            free(as->endpoint[k].extra);
        }
        free(as->endpoint);
      }

      free(ifp->altsetting);
    }

    free(cf->interface);
  }

  free(dev->config);
}
/** @} */
