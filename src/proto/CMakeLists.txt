# Includes
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${SHARED_DIR}
                     )

# Find pthreads
find_package(Threads REQUIRED)

# Targets
set(sources_c protocol.c
              protobase.c
              ${SHARED_DIR}/common.c
              )

set(sources   protocol.cpp
              socket.cpp
              protobase.c
              ${SHARED_DIR}/common.c
              )

set(headers_c protocol.h
              protobase.h
              )

set(headers   protocol.hpp
              socket.hpp
              )

add_library(urpc    SHARED ${sources_c} ${headers_c})
set_target_properties(urpc PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(urpc PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.0 SOVERSION 1)

add_library(urpc_pp SHARED ${sources} ${headers})
set_target_properties(urpc_pp PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(urpc_pp PROPERTIES VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.0 SOVERSION 1)

# Install
install( TARGETS urpc urpc_pp
         LIBRARY DESTINATION ${LIBDIR}
         )

install( FILES ${headers} ${headers_c}
         DESTINATION include/${PROJECT_NAME}
         )

