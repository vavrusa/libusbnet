# Includes
set(SHARED_DIR ${CMAKE_SOURCE_DIR}/src/shared)
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${SHARED_DIR}
                     )

# Targets
set(sources   usbnet.c
              ${SHARED_DIR}/protocol.c
              )
set(headers   usbnet.h
              ${SHARED_DIR}/common.h
              ${SHARED_DIR}/protocol.h
              )

# Find pthreads
find_package(Threads REQUIRED)

add_library(usbnet SHARED ${sources} ${headers})

# Static library
add_library(usbnet_static STATIC ${sources} ${headers})
set_target_properties(usbnet_static PROPERTIES OUTPUT_NAME "usbnet")

# Prevent clobbering each other during the build
set_target_properties(usbnet PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(usbnet_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Client/Server
add_subdirectory(client)
add_subdirectory(server)

# Dependencies
target_link_libraries(usbnet ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(usbnet ${LIBUSB_LIBRARIES})

# Install
install( TARGETS usbnet
         LIBRARY DESTINATION lib
         )

install( TARGETS usbnet_static
         ARCHIVE DESTINATION lib
         )

install( FILES ${headers}
         DESTINATION include/${PROJECT_NAME}
         )

