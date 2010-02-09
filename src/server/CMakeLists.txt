# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${SHARED_DIR}
                     )

# Targets
set(sources   usbexportd.cpp
              usbservice.cpp
              serversocket.cpp
              )
set(headers   serversocket.hpp
              usbservice.hpp
              )

# Build executable
add_executable(usbexportd ${sources} ${headers})

# Prevent clobbering each other during the build
set_target_properties(usbexportd PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(usbexportd ${LIBUSB_LIBRARIES} urpc_pp)

# Install
install( TARGETS usbexportd
         RUNTIME DESTINATION bin
         )

install( FILES ${headers}
         DESTINATION include/${PROJECT_NAME}
         )
