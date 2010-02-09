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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <pthread.h>
#include "usbnet.h"
#include "usbproto.h"
#include "common.h"

// Global call lock
// TODO: make more efficient call exclusion
static pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;

static void call_lock() {
   pthread_mutex_lock(&__mutex);
}

static void call_release() {
   pthread_mutex_unlock(&__mutex);
}

// Remote socket filedescriptor
static int __remote_fd = -1;

// Remote USB busses with devices
static struct usb_bus* __remote_bus = 0;

// Return remote filedescriptor
static int get_remote() {

   // Check fd
   if(__remote_fd != -1)
      return __remote_fd;

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

void usb_init(void)
{
   // Initialize remote fd
   call_lock();
   int fd = get_remote();

   // Create buffer
   char buf[PACKET_MINSIZE];
   packet_t pkt = pkt_create(buf, PACKET_MINSIZE);
   pkt_init(&pkt, UsbInit);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));
   call_release();
   printf("%s: called\n", __func__);
}

int usb_find_busses(void)
{
  // Get remote fd
   call_lock();
   int fd = get_remote();

   // Create buffer
   char buf[32];
   packet_t pkt = pkt_create(buf, 32);
   pkt_init(&pkt, UsbFindBusses);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get number of changes
   int res = 0;
   sym_t sym;
   if(pkt_recv(fd, &pkt) > 0) {
      pkt_begin(&pkt, &sym);
      if(sym.cur != pkt_end(&pkt)) {
         if(sym.type == IntegerType)
            res = as_uint(sym.val, sym.len);
      }
   }

   // Return remote result
   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

int usb_find_devices(void)
{
   NOT_IMPLEMENTED

   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Create buffer
   char buf[4096];
   packet_t pkt = pkt_create(buf, 4096);
   pkt_init(&pkt, UsbFindDevices);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get number of changes
   int res = 0;
   sym_t sym;
   if(pkt_recv(fd, &pkt) > 0) {
      pkt_begin(&pkt, &sym);

      // Get return value
      if(sym.type == IntegerType) {
         res = as_uint(sym.val, sym.len);
         sym_next(&sym);
      }

      // Allocate structures
      struct usb_bus vbus;
      vbus.next = __remote_bus;
      struct usb_bus* rbus = &vbus;

      // Get busses
      for(;;) {

         // Evaluate
         if(sym.type == StructureType) {
            sym_enter(&sym);

            // Allocate bus
            uint8_t is_new = 0;
            if(rbus->next == NULL) {

               // Allocate next item
               struct usb_bus* nbus = malloc(sizeof(struct usb_bus));
               memset(nbus, 0, sizeof(struct usb_bus));
               rbus->next = nbus;
               nbus->prev = rbus;
               rbus = nbus;
               is_new = 1;
            }
            else
               rbus = rbus->next;

            // Read dirname
            if(sym.type == OctetType) {
               strcpy(rbus->dirname, as_string(sym.val, sym.len));
               sym_next(&sym);
            }

            // Read location
            if(sym.type == IntegerType) {
               rbus->location = as_int(sym.val, sym.len);
               if(is_new) {
                  printf("%s: new bus id: %03d\n", __func__, rbus->location);
               }
               sym_next(&sym);
            }

            // Read devices
            struct usb_device vdev;
            vdev.next = rbus->devices;
            struct usb_device* dev = &vdev;
            while(sym.type == SequenceType) {
               sym_enter(&sym);

               // Initialize
               uint8_t is_new = 0;
               if(dev->next == NULL) {
                  dev->next = malloc(sizeof(struct usb_device));
                  memset(dev->next, 0, sizeof(struct usb_device));
                  dev->next->bus = rbus;
                  if(dev != &vdev)
                     dev->next->prev = dev;
                  if(rbus->devices == NULL)
                     rbus->devices = dev->next;
                  is_new = 1;
               }

               dev = dev->next;

               // Read filename
               if(sym.type == OctetType) {
                  strcpy(dev->filename, as_string(sym.val, sym.len));
                  sym_next(&sym);
               }

               // Read description
               if(sym.type == RawType) {
                  memcpy(&dev->descriptor, sym.val, sym.len);
                  sym_next(&sym);
               }

               // Read devnum
               if(sym.type == IntegerType) {
                  dev->devnum = as_int(sym.val, sym.len);
                  if(is_new) {
                     printf("%s: new device id: %03d vendor_id: %04x product_id: %04x\n",
                            __func__, dev->devnum, dev->descriptor.idVendor, dev->descriptor.idProduct);
                  }
                  sym_next(&sym);
               }
            }

            // Free unused devices
            while(dev->next != NULL) {
               struct usb_device* ddev = dev->next;
               printf("%s: deleting device id: %03d\n", __func__, ddev->devnum);
               dev->next = ddev->next;
               free(ddev);
            }

         }
         else {
            printf("%s: unexpected symbol 0x%02x\n", __func__, sym.type);
            sym_next(&sym);
         }

         // Next symbol
         if(sym.next == pkt_end(&pkt))
            break;
      }

      // Deallocate unnecessary busses
      while(rbus->next != NULL) {
         printf("%s: deleting bus %03d\n", __func__, rbus->next->location);
         struct usb_bus* bus = rbus->next;
         rbus->next = bus->next;
         free(bus);
      }

      // Save busses
      __remote_bus = vbus.next;
   }

   // Return remote result
   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

struct usb_bus* usb_get_busses(void)
{
   // TODO: merge both Local/Remote bus in future
   printf("%s: returned %p\n", __func__, __remote_bus);
   return __remote_bus;
}

/* libusb(2):
 * Device operations.
 */

usb_dev_handle *usb_open(struct usb_device *dev)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbOpen);
   pkt_append(&pkt, IntegerType, sizeof(dev->bus->location), &dev->bus->location);
   pkt_append(&pkt, IntegerType, sizeof(dev->devnum),        &dev->devnum);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get response
   int res = -1, devfd = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbOpen) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
         sym_next(&sym);
      }
      if(sym.type == IntegerType) {
          devfd = as_int(sym.val, sym.len);
      }
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
   printf("%s: returned %d (on fd %d)\n", __func__, res, devfd);
   return udev;
}

int usb_close(usb_dev_handle *dev)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClose);
   pkt_append(&pkt, IntegerType, sizeof(dev->fd),  &dev->fd);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Free device
   free(dev);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbClose) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
      }
   }

   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

int usb_claim_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClaimInterface);
   pkt_append(&pkt, IntegerType, sizeof(dev->fd),  &dev->fd);
   pkt_append(&pkt, IntegerType, sizeof(int),      &interface);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbClaimInterface) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
      }
   }

   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

int usb_release_interface(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbReleaseInterface);
   pkt_append(&pkt, IntegerType, sizeof(dev->fd),  &dev->fd);
   pkt_append(&pkt, IntegerType, sizeof(int),      &interface);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbReleaseInterface) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
      }
   }

   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

int usb_detach_kernel_driver_np(usb_dev_handle *dev, int interface)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbDetachKernelDriver);
   pkt_append(&pkt, IntegerType, sizeof(dev->fd),  &dev->fd);
   pkt_append(&pkt, IntegerType, sizeof(int),      &interface);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbDetachKernelDriver) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
      }
   }

   call_release();
   printf("%s: returned %d\n", __func__, res);
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
   int fd = get_remote();

   // Prepare packet
   packet_t* pkt = pkt_new(size + 128);
   pkt_init(pkt, UsbControlMsg);
   pkt_append(pkt, IntegerType, sizeof(dev->fd), &dev->fd);
   pkt_append(pkt, IntegerType, sizeof(int), &requesttype);
   pkt_append(pkt, IntegerType, sizeof(int), &request);
   pkt_append(pkt, IntegerType, sizeof(int), &value);
   pkt_append(pkt, IntegerType, sizeof(int), &index);
   pkt_append(pkt, OctetType,   size,        bytes);
   pkt_append(pkt, IntegerType, sizeof(int), &timeout);
   pkt_send(fd, pkt->buf, pkt_size(pkt));

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt->buf[0] == UsbControlMsg) {
      sym_t sym;
      pkt_begin(pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
         sym_next(&sym);
      }
      if(sym.type == OctetType) {
         if(res > 0) {
            int minlen = (res > size) ? size : res;
            memcpy(bytes, sym.val, minlen);
         }
      }
   }

   // Return response
   pkt_del(pkt);
   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

/* libusb(4):
 * Bulk transfers.
 */

int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Prepare packet
   packet_t* pkt = pkt_new(size + 128);
   pkt_init(pkt, UsbBulkRead);
   pkt_append(pkt, IntegerType, sizeof(dev->fd), &dev->fd);
   pkt_append(pkt, IntegerType, sizeof(int), &ep);
   pkt_append(pkt, IntegerType, sizeof(int), &size);
   pkt_append(pkt, IntegerType, sizeof(int), &timeout);
   pkt_send(fd, pkt->buf, pkt_size(pkt));

   // Get response
   int res = -1;
   unsigned char op = 0x00;
   if(pkt_recv(fd, pkt) > 0) {
      op = pkt->buf[0];
      sym_t sym;
      pkt_begin(pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
         sym_next(&sym);
      }
      if(sym.type == OctetType) {
         if(res > 0) {
            int minlen = (res > size) ? size : res;
            memcpy(bytes, sym.val, minlen);
         }
      }
   }

   // Return response
   pkt_del(pkt);
   call_release();
   printf("%s: returned %d (op 0x%02x)\n", __func__, res, op);
   return res;
}

int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout)
{
   // Get remote fd
   call_lock();
   int fd = get_remote();

   // Prepare packet
   packet_t* pkt = pkt_new(size + 128);
   pkt_init(pkt, UsbBulkWrite);
   pkt_append(pkt, IntegerType, sizeof(dev->fd), &dev->fd);
   pkt_append(pkt, IntegerType, sizeof(int), &ep);
   pkt_append(pkt, OctetType,   size,        bytes);
   pkt_append(pkt, IntegerType, sizeof(int), &timeout);
   pkt_send(fd, pkt->buf, pkt_size(pkt));

   // Get response
   int res = -1;
   if(pkt_recv(fd, pkt) > 0 && pkt->buf[0] == UsbBulkWrite) {
      sym_t sym;
      pkt_begin(pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
         sym_next(&sym);
      }
   }

   // Return response
   pkt_del(pkt);
   call_release();
   printf("%s: returned %d\n", __func__, res);
   return res;
}

/* Imported from libusb-0.1.12 for forward compatibility with libusb-1.0.
 * This overrides libusb-0.1 as well as libusb-1.0 calls.
 * @see libusb-0.1.12/usb.c:219
 * @see libusb-0.1.12/usb.c:230
 */
int usb_get_string(usb_dev_handle *dev, int index, int langid, char *buf,
        size_t buflen)
{
  /*
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
