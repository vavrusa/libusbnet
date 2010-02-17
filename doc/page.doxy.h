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
*/

/* Modules. */

/** \defgroup proto Protocol definitions and C API.
  * \defgroup protocpp Protocol abstraction in C++.
  * \defgroup client Client executable wrapper.
  * \defgroup server Server executable.
  * \defgroup libusbnet Protocol implementation for libusb.
  */
