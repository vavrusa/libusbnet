# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     )

# Targets
set(sources_c protocol.c
              )

set(sources   protocol.cpp
              socket.cpp
              )

set(headers_c protocol.h
              )

set(headers   protocol.hpp
              socket.hpp
              )

add_library(urpc    SHARED ${sources_c} ${headers_c})
add_library(urpc_pp SHARED ${sources_c} ${headers_c} ${sources} ${headers})

# Install
install( TARGETS urpc urpc_pp
         LIBRARY DESTINATION lib
         )

install( FILES ${headers} ${headers_c}
         DESTINATION include/${PROJECT_NAME}
         )

