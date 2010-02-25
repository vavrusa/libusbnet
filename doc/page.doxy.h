/**
    \mainpage libusbnet API documentation.

    libusbnet is a libusb network proxy, which enables
    communication with USB over TCP/IP network.

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
    See "usbnet --help".
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
      call_lock(); // Synchronize
      int fd = init_hostfd(); // Get remote socket descriptor
    \endcode
    Send packet with opcode and paramter.
    \code
      char buf[32]; // Create static packet (32B)
      Packet pkt = pkt_create(buf, PACKET_MINSIZE); // Create packet
      pkt_init(&pkt, OpCode1); // Initialize packet
      pkt_addint(&pkt, param); // Append parameter
      pkt_send(&pkt, fd); // Send packet
    \endcode
    Receive and store result value.
    \code
      Iterator it;
      int res = -1;
      if(pkt_recv(fd, &pkt) > 0) { // Receive packet
         pkt_begin(&pkt, &it); // Initialize iterator
         res = iter_getint(&it); // Save result
      }
    \endcode
    Release call lock and return value.
    \code
      call_release(); // Release lock
      return res;
    }
    \endcode
*/

/* Modules. */

/** \defgroup proto Protocol definitions and C API.
  * \defgroup protocpp Protocol abstraction in C++.
  * \defgroup client Client executable wrapper.
  * \defgroup server Server executable.
  * \defgroup libusbnet Protocol implementation for libusb.
  */

