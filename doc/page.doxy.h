/**
    \mainpage libusbnet API documentation.

    libusbnet is a libusb network proxy, which enables
    communication with USB over TCP/IP network.

    <h2>Installation</h2>
    Unpack source package or clone latest git. Installation requires CMake >= 2.6, GCC and libusb development headers.
    \code
    user@localhost# mkdir build && cd build # Create build directory
    user@localhost# cmake ..                # Create Makefiles using CMake
    user@localhost# make                    # Build
    user@localhost# sudo make install       # Install
    \endcode

    <h2>Project modules</h2>

    - \ref proto.
      - \ref protocpp
    - \ref client
    - \ref server
    - \ref libusbnet

    <h2>Example usage (probing remote USB bus)</h2>
    \code
    john@server# usbexportd
    jack@client# usbnet -h server:22222 -l libusbnet.so "lsusb"
    \endcode
    See <strong>usbnet --help</strong>.
    <br/>
    <table>
      <tr>
        <td style="border: 0;">\copydoc proto_page</td>
        <td style="border: 0; border-left: 1px dashed #ccc; vertical-align: top;">\copydoc protopp_page</td>
      </tr>
    </table>

    <h2>Wrapping function declaration</h2>
    - Include original declaration and reimplement
    - Build as shared library
    - Preload with "usbnet".

    <h3>Example reimplementation</h3>
    Initialize remote socket descriptor and synchronize call to ensure thread-safe result.
    \code
    int function_call(int param)
    {
      Packet* pkt_claim();    // Claim shared buffer (efficient and synchronous)
      int fd = session_get(); // Get remote socket descriptor
    \endcode
    Send packet with opcode and paramter.
    \code
      pkt_init(pkt, OpCode1); // Initialize packet
      pkt_addint(pkt, param); // Append parameter
      pkt_send(pkt, fd);      // Send packet
    \endcode
    Receive and store result value.
    \code
      Iterator it;
      int res = -1;
      if(pkt_recv(fd, pkt) > 0) { // Receive packet
         pkt_begin(pkt, &it);     // Initialize iterator
         res = iter_getint(&it);  // Save result
      }
    \endcode
    Release call lock and return value.
    \code
      pkt_release(); // Release shared buffer
      return res;
    }
    \endcode
    <br/>
    Marek Vavrusa <marek@vavrusa.com><br/>
    Zdenek Vasicek <vasicek@fit.vutbr.cz>
*/

/* Modules. */

/** \defgroup proto Protocol definitions and C API.
  * \defgroup protocpp Protocol abstraction in C++.
  * \defgroup client Client executable wrapper.
  * \defgroup server Server executable.
  * \defgroup libusbnet Protocol implementation for libusb.
  */

