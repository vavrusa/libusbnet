# Includes
project(libusbnet)
set(SHARED_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/shared)
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/proto
                     ${SHARED_DIR}
                     )

# Targets
set(sources   usbnet.c
              )
set(headers   usbnet.h
              ${SHARED_DIR}/common.h
              )

# Client/Server
add_subdirectory(proto)
add_subdirectory(client)
add_subdirectory(server)

# Create library
add_library(usbnet SHARED ${sources} ${headers})
set_target_properties(usbnet PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(usbnet PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.0 SOVERSION 1)

# Dependencies
target_link_libraries(usbnet ${LIBUSB_LIBRARIES} urpc)

# Install
install( TARGETS usbnet
         LIBRARY DESTINATION ${LIBDIR}
         )

install( FILES ${headers}
         DESTINATION include/${PROJECT_NAME}
         )

