# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     )

# Targets
set(sources   usbexportd.cpp
              socket.cpp
              )
set(headers   socket.hpp
              )

add_executable(usbexportd ${sources})

# Prevent clobbering each other during the build
set_target_properties(usbexportd PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(usbexportd ${LIBUSB_LIBRARIES})

# Install
install( TARGETS usbexportd
         RUNTIME DESTINATION bin
         )
