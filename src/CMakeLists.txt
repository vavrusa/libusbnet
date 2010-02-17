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

# Find pthreads
find_package(Threads REQUIRED)

# Client/Server
add_subdirectory(proto)
add_subdirectory(client)
add_subdirectory(server)

# Create library
add_library(usbnet SHARED ${sources} ${headers})

# Dependencies
target_link_libraries(usbnet ${CMAKE_THREAD_LIBS_INIT} ${LIBUSB_LIBRARIES} urpc)

# Install
install( TARGETS usbnet
         LIBRARY DESTINATION lib
         )

install( FILES ${headers}
         DESTINATION include/${PROJECT_NAME}
         )

