# Try to find libusb
# Once done this defines
#
#  LIBUSB_FOUND - system has libusb
#  LIBUSB_INCLUDE_DIR - the libusb include directory
#  LIBUSB_LIBRARIES - Link these to use libusb

find_package(PkgConfig)
pkg_check_modules(PC_LIBUSB QUIET libusb)
set(LIBUSB_DEFINITIONS ${PC_LIBUSB_CFLAGS_OTHER})

find_path(LIBUSB_INCLUDE_DIR usb.h
          HINTS ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDE_DIRS}
          PATH_SUFFIXES libusb )

find_library(LIBUSB_LIBRARY NAMES usb 
             HINTS ${PC_LIBUSB_LIBDIR} ${PC_LIBUSB_LIBRARY_DIRS} )

set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY} )
set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBUSB_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibUsb DEFAULT_MSG
                                  LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)

mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY )

INCLUDE(CheckCSourceCompiles)
CHECK_C_SOURCE_COMPILES("
#include <usb.h>
int usb_bulk_write(usb_dev_handle *dev, int ep, const char *bytes, int size,
        int timeout){}
int usb_interrupt_write(usb_dev_handle *dev, int ep, const char *bytes,
	int size, int timeout){}
int main()
{
    usb_init();
    return 0;
}
" LIBUSB_CONST_BUFFERS)

CHECK_C_SOURCE_COMPILES("
#include <usb.h>
static void usb_destroy_configuration(struct usb_device *dev) {
}
int main()
{
    usb_destroy_configuration(NULL);
    return 0;
}
" MISSING_USB_DESTROY_CONFIGURATION)
