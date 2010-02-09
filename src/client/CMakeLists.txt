# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     )

# Targets
set(sources   usbnet.cpp
              clientsocket.cpp
              ${SHARED_DIR}/cmdflags.cpp
              )
set(headers   clientsocket.hpp
              )

add_executable(usbnet-wrapper ${sources} ${headers})

# Prevent clobbering each other during the build
set_target_properties(usbnet-wrapper PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(usbnet-wrapper PROPERTIES OUTPUT_NAME "usbnet")

# Dependencies
target_link_libraries(usbnet-wrapper ${LIBUSB_LIBRARIES} urpc_pp)

# Install
install( TARGETS usbnet-wrapper
         RUNTIME DESTINATION bin
         )

install( FILES ${headers}
         DESTINATION include/${PROJECT_NAME}
         )
