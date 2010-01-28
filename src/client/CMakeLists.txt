# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${SHARED_DIR}
                     )

# Targets
set(sources   usbnet.cpp
              clientsocket.cpp
              ${SHARED_DIR}/socket.cpp
              )
set(headers   clientsocket.hpp
              ${SHARED_DIR}/socket.hpp
              )

add_executable(usbnet-wrapper ${sources} ${headers})

# Prevent clobbering each other during the build
set_target_properties(usbnet-wrapper PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(usbnet-wrapper PROPERTIES OUTPUT_NAME "usbnet")

# Dependencies
target_link_libraries(usbnet-wrapper ${LIBUSB_LIBRARIES})

# Install
install( TARGETS usbnet-wrapper
         RUNTIME DESTINATION bin
         )
