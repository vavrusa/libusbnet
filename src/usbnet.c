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
#include "usbnet.h"
#include "common.h"
#include "protocol.h"

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
   static void (*func)(void) = NULL;
   READ_SYM(func, "usb_init")

   // Initialize remote fd
   int fd = get_remote();

   // Create buffer
   char buf[PACKET_MINSIZE];
   packet_t pkt = pkt_create(buf, PACKET_MINSIZE);
   pkt_init(&pkt, UsbInit);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));
}

int usb_find_busses(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_busses")

   // Get remote fd
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

         printf("%s: returned %d\n", __func__, res);
      }
   }

   // Return remote result
   return res;
}

int usb_find_devices(void)
{
   static int (*func)(void) = NULL;
   READ_SYM(func, "usb_find_devices")
   NOT_IMPLEMENTED

   // Get remote fd
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
         printf("sym 0x%02x len %d\n", sym.type, sym.len);
         if(sym.type == StructureType) {
            printf("<entering struct>\n");
            sym_enter(&sym);

            // Allocate bus
            if(rbus->next == NULL) {

               // Allocate next item
               printf("<allocating new bus>\n");
               struct usb_bus* nbus = malloc(sizeof(struct usb_bus));
               memset(nbus, 0, sizeof(struct usb_bus));
               rbus->next = nbus;
               nbus->prev = rbus;
               rbus = nbus;
            }
            else
               rbus = rbus->next;

            // Read dirname
            if(sym.type == OctetType) {
               strcpy(rbus->dirname, as_string(sym.val, sym.len));
               printf(" dirname: %s\n", rbus->dirname);
               sym_next(&sym);
            }

            // Read location
            if(sym.type == IntegerType) {
               rbus->location = as_uint(sym.val, sym.len);
               printf(" location: %d\n", rbus->location);
               sym_next(&sym);
            }

            // Read devices
            struct usb_device vdev;
            vdev.next = rbus->devices;
            struct usb_device* dev = &vdev;
            while(sym.type == SequenceType) {
               sym_enter(&sym);
               printf(" <device>\n");

               // Initialize
               if(dev->next == NULL) {
                  printf(" <allocating new device>\n");
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
               if(sym.type == OctetType) {
                  strcpy(dev->filename, as_string(sym.val, sym.len));
                  printf("  filename: %s\n", dev->filename);
                  sym_next(&sym);
               }

               // Read description
               if(sym.type == RawType) {
                  memcpy(&dev->descriptor, sym.val, sym.len);
                  printf("  description: .idVendor %x .idProduct %x\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
                  sym_next(&sym);
               }

               // Read devnum
               if(sym.type == IntegerType) {
                  dev->devnum = as_uint(sym.val, sym.len);
                  printf("  number: %u\n", dev->devnum);
                  sym_next(&sym);
               }
            }

            // Free unused devices
            while(dev->next != NULL) {
               struct usb_device* ddev = dev->next;
               printf(" <deallocating device %d>\n", ddev->devnum);
               dev->next = ddev->next;
               free(ddev);
            }

         }
         else {
            printf("<unexpected symbol 0x%02x>\n", sym.type);
            sym_next(&sym);
         }

         // Next symbol
         if(sym.next == pkt_end(&pkt))
            break;
      }

      // Deallocate unnecessary busses
      while(rbus->next != NULL) {
         printf("<deallocating bus %d>\n", rbus->next->location);
         struct usb_bus* bus = rbus->next;
         rbus->next = bus->next;
         free(bus);
      }

      // Save busses
      __remote_bus = vbus.next;
   }

   // Return remote result
   printf("%s: returned %d\n", __func__, res);
   return res;
}

struct usb_bus* usb_get_busses(void)
{
   static struct usb_bus* (*func)(void) = NULL;
   READ_SYM(func, "usb_get_busses")

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
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbOpen);
   pkt_append(&pkt, IntegerType, sizeof(dev->bus->location), &dev->bus->location);
   pkt_append(&pkt, IntegerType, sizeof(dev->devnum),        &dev->devnum);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));
   printf("%s: %u:%u\n", __func__, dev->bus->location, dev->devnum);

   // Get response
   int res = -1;
   if(pkt_recv(fd, &pkt) > 0 && pkt.buf[0] == UsbOpen) {
      sym_t sym;
      pkt_begin(&pkt, &sym);
      if(sym.type == IntegerType) {
         res = as_int(sym.val, sym.len);
      }
   }

   // Evaluate
   printf("%s: returned %d\n", __func__, res);
   if(res < 0)
      return NULL;

   usb_dev_handle* udev = malloc(sizeof(usb_dev_handle));
   udev->fd = -1;
   udev->device = dev;
   udev->bus = dev->bus;
   udev->config = udev->interface = udev->altsetting = -1;

   return udev;
}

int usb_close(usb_dev_handle *dev)
{
   // Get remote fd
   int fd = get_remote();

   // Send packet
   char buf[255];
   packet_t pkt = pkt_create(buf, 255);
   pkt_init(&pkt, UsbClose);
   pkt_append(&pkt, IntegerType, sizeof(dev->bus->location),  &dev->bus->location);
   pkt_append(&pkt, IntegerType, sizeof(dev->device->devnum), &dev->device->devnum);
   pkt_send(fd, pkt.buf, pkt_size(&pkt));
   printf("%s: %u:%u\n", __func__, dev->bus->location, dev->device->devnum);

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
   int fd = get_remote();

   // Prepare packet
   packet_t* pkt = pkt_new(size + 128);
   pkt_init(pkt, UsbControlMsg);
   pkt_append(pkt, IntegerType, sizeof(dev->bus->location),  &dev->bus->location);
   pkt_append(pkt, IntegerType, sizeof(dev->device->devnum), &dev->device->devnum);
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
