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
#include <pthread.h>
#include <sys/shm.h>
#include <netinet/in.h>
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

/* Call serialization.
 */
static pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;
static void call_lock() {
   pthread_mutex_lock(&__mutex);
}

static void call_release() {
   pthread_mutex_unlock(&__mutex);
}

//! Remote socket filedescriptor
static int __remote_fd = -1;

//! Remote USB busses with devices
static struct usb_bus* __remote_bus = NULL;
extern struct usb_bus* usb_busses;

/* Global variables manipulators.
 */

/** Free allocated busses and virtual devices.
  * Function is called on executable exit (atexit()).
  */
void deinit_hostfd() {
   debug_msg("freeing busses ...");

   // Unhook global variable
   usb_busses = 0;

   // Free busses
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

/** Return host socket descriptor.
  * Retrieve socket descriptor from SHM if not present
  * and hook exit function for cleanup.
  */
int init_hostfd() {

   // Check exit function
   static char exitf_hooked = 0;
   if(!exitf_hooked) {
      atexit(&deinit_hostfd);
      exitf_hooked = 1;
   }

   // Check fd
   if(__remote_fd == -1) {

      // Get fd from SHM
      int shm_id = 0;
      printf("IPC: accessing segment at key 0x%x (%d bytes)\n", SHM_KEY, SHM_SIZE);
      if((shm_id = shmget(SHM_KEY, SHM_SIZE, 0666)) != -1) {

         // Attach segment and read fd
         printf("IPC: attaching segment %d\n", shm_id);
         void* shm_addr = NULL;
         if ((shm_addr = shmat(shm_id, NULL, 0)) != (void *) -1) {

            // Read fd
            __remote_fd = *((int*) shm_addr);

            // Detach
            shmdt(shm_addr);
         }
      }

      // Check resulting fd
      printf("IPC: remote fd is %d\n", __remote_fd);
   }

   // Check peer name - keep-alive
   struct sockaddr_in addr;
   int len = sizeof(addr);
   if(getpeername(__remote_fd, (struct sockaddr*)& addr, &len) < 0)
      __remote_fd = -1;

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
   // Initialize remote fd
   call_lock();
   int fd = init_hostfd();

   // Create buffer
   char buf[PACKET_MINSIZE];
   Packet pkt = pkt_create(buf, PACKET_MINSIZE);
   pkt_init(&pkt, UsbInit);
   pkt_send(&pkt, fd);
   call_release();

   // Initialize locally
   debug_msg("called");
}

/** Find busses on remote host.
    \warning Does not transfer busses to local virtual bus list.
  */
int usb_find_busses(void)
{
  // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Create buffer
   char buf[32];
   Packet pkt = pkt_create(buf, 32);
   pkt_init(&pkt, UsbFindBusses);
   pkt_send(&pkt, fd);

   // Get number of changes
   int res = 0;
   Iterator it;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbFindBusses) {
      if(pkt_begin(&pkt, &it) != NULL) {
         res = iter_getint(&it);
      }
   }

   // Return remote result
   call_release();
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
   call_lock();
   int fd = init_hostfd();

   // Create buffer
   char buf[8192];
   Packet pkt = pkt_create(buf, 8192);
   pkt_init(&pkt, UsbFindDevices);
   pkt_send(&pkt, fd);

   // Get number of changes
   int res = 0;
   Iterator it;
   if(pkt_recv(fd, &pkt) > 0) {
      pkt_begin(&pkt, &it);

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

               /** Read descriptor
                 * \todo Byte-order for int16/32 members in this and other octet transfers.
                 */
               memcpy(&dev->descriptor, it.val, it.len);
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

                  memcpy(cfg, it.val, szlen);

                  // Allocate interfaces
                  cfg->interface = NULL;
                  if(cfg->bNumInterfaces > 0) {
                     cfg->interface = malloc(cfg->bNumInterfaces * sizeof(struct usb_interface));
                  }

                  //! \todo Implement usb_device extra interfaces - are they needed?
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

                           memcpy(endpoint, it.val, szlen);
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

               log_msg("Bus %s Device %s: ID %04x:%04x", rbus->dirname, dev->filename, dev->descriptor.idVendor, dev->descriptor.idProduct);
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
         debug_msg("overriding global usb_busses from %p to %p", usb_busses, vbus.next);
      }

      __remote_bus = vbus.next;
      usb_busses = __remote_bus;
   }

   // Return remote result
   call_release();
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
   call_lock();
   int fd = init_hostfd();

   // Send packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbOpen);
   pkt_adduint(&pkt, dev->bus->location);
   pkt_adduint(&pkt, dev->devnum);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1, devfd = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbOpen) {
      Iterator it;
      pkt_begin(&pkt, &it);
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

   call_release();
   debug_msg("returned %d (fd %d)", res, devfd);
   return udev;
}

int usb_close(usb_dev_handle *dev)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Send packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClose);
   pkt_addint(&pkt, dev->fd);
   pkt_send(&pkt, fd);

   // Free device
   free(dev);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbClose) {
      Iterator it;
      pkt_begin(&pkt, &it);
      res = iter_getint(&it);
   }

   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_set_configuration(usb_dev_handle *dev, int configuration)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbSetConfiguration);
   pkt_addint(&pkt, dev->fd);
   pkt_addint(&pkt, configuration);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbSetConfiguration) {
      Iterator it;
      pkt_begin(&pkt, &it);

      // Read result
      res = iter_getint(&it);

      // Read callback configuration
      configuration = iter_getint(&it);
   }

   // Save configuration
   dev->config = configuration;

   // Return response
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_set_altinterface(usb_dev_handle *dev, int alternate)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbSetAltInterface);
   pkt_addint(&pkt, dev->fd);
   pkt_addint(&pkt, alternate);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbSetAltInterface) {
      Iterator it;
      pkt_begin(&pkt, &it);

      // Read result
      res = iter_getint(&it);

      // Read callback configuration
      alternate = iter_getint(&it);
   }

   // Save configuration
   dev->altsetting = alternate;

   // Return response
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_resetep(usb_dev_handle *dev, unsigned int ep)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbResetEp);
   pkt_addint(&pkt,  dev->fd);
   pkt_adduint(&pkt, ep);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbResetEp) {
      Iterator it;
      pkt_begin(&pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_clear_halt(usb_dev_handle *dev, unsigned int ep)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClearHalt);
   pkt_addint(&pkt, dev->fd);
   pkt_adduint(&pkt, ep);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbClearHalt) {
      Iterator it;
      pkt_begin(&pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_reset(usb_dev_handle *dev)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbReset);
   pkt_addint(&pkt, dev->fd);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbReset) {
      Iterator it;
      pkt_begin(&pkt, &it);

      // Read result
      res = iter_getint(&it);
   }

   // Return response
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_claim_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Send packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClaimInterface);
   pkt_addint(&pkt, dev->fd);
   pkt_addint(&pkt, interface);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbClaimInterface) {
      Iterator it;
      pkt_begin(&pkt, &it);
      res = iter_getint(&it);
   }

   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

int usb_release_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Send packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbReleaseInterface);
   pkt_addint(&pkt, dev->fd);
   pkt_addint(&pkt, interface);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbReleaseInterface) {
      Iterator it;
      pkt_begin(&pkt, &it);
      res = iter_getint(&it);
   }

   call_release();
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
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   Packet* pkt = pkt_new(size + 128, UsbControlMsg);
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
   pkt_free(pkt);
   call_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(4):
 * Bulk transfers.
 */

int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   Packet* pkt = pkt_new(size + 128, UsbBulkRead);
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
   pkt_free(pkt);
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_bulk_write(usb_dev_handle *dev, int ep, usb_buf_t bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   Packet* pkt = pkt_new(size + 128, UsbBulkWrite);
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
   pkt_free(pkt);
   call_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(5):
 * Interrupt transfers.
 */
int usb_interrupt_write(usb_dev_handle *dev, int ep, usb_buf_t bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   Packet* pkt = pkt_new(size + 128, UsbInterruptWrite);
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
   pkt_free(pkt);
   call_release();
   debug_msg("returned %d", res);
   return res;
}

int usb_interrupt_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Prepare packet
   Packet* pkt = pkt_new(size + 128, UsbInterruptRead);
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
   pkt_free(pkt);
   call_release();
   debug_msg("returned %d", res);
   return res;
}

/* libusb(6):
 * Non-portable.
 */

int usb_detach_kernel_driver_np(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = init_hostfd();

   // Send packet
   char buf[255];
   Packet pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbDetachKernelDriver);
   pkt_addint(&pkt, dev->fd);
   pkt_addint(&pkt, interface);
   pkt_send(&pkt, fd);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt_op(&pkt) == UsbDetachKernelDriver) {
      Iterator it;
      pkt_begin(&pkt, &it);
      res = iter_getint(&it);
   }

   call_release();
   debug_msg("returned %d", res);
   return res;

}

/** \private
 * Imported from libusb-0.1.12 for forward compatibility with libusb-1.0.
 * This overrides libusb-0.1 as well as libusb-1.0 calls.
 * @see libusb-0.1.12/usb.c:219
 * @see libusb-0.1.12/usb.c:230
 */
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
