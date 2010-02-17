/**
    \mainpage libusbnet API documentation.

    libusbnet is a libusb network proxy, which enables
    communication with USB over TCP/IP network.

    \section modlist Project modules.

    - Protocol definitions and C API.
      - Protocol abstraction in C++.
    - Client executable wrapper.
    - Server executable.
    - Protocol implementation for libusb.

    \section usage Example usage (probing remote USB bus).
    \code
    server-user@server# usbexportd
    client-user@client: usbnet -h server:22222 -l libusbnet.so "lsusb"
    \endcode
*/

/* Modules. */

/** \defgroup proto Protocol definitions and C API.
  * \defgroup protocpp Protocol abstraction in C++.
  * \defgroup client Client executable wrapper.
  * \defgroup server Server executable.
  * \defgroup libusbnet Protocol implementation for libusb.
  */
