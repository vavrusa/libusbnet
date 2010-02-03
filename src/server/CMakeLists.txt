# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/../shared
                     )

# Targets
set(sources   usbexportd.cpp
              usbservice.cpp
              serversocket.cpp
              ${SHARED_DIR}/socket.cpp
              ${SHARED_DIR}/protocol.c
              ${SHARED_DIR}/protocol.cpp
              )
set(headers   serversocket.hpp
              usbservice.hpp
              ${SHARED_DIR}/socket.hpp
              ${SHARED_DIR}/protocol.hpp
              )

add_executable(usbexportd ${sources} ${headers})

# Prevent clobbering each other during the build
set_target_properties(usbexportd PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(usbexportd ${LIBUSB_LIBRARIES})

# Install
install( TARGETS usbexportd
         RUNTIME DESTINATION bin
         )
