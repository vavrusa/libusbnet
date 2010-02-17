# Doxygen documentation
# Required variables set: DOCUMENTATION_DIR
# Looks up Doxygen config in DOCUMENTATION_DIR/Doxyfile.in
option(DOCUMENTATION "Generate API documentation with Doxygen" ON)

# Find Doxygen
find_package(Doxygen)
if(DOCUMENTATION AND DOXYGEN_FOUND)

   # Set variables
   set(PROJECT ${CMAKE_PROJECT_NAME})
   set(VERSION ${VERSION_STRING})
   set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

   # Find doxy config
   message(STATUS "Doxygen found.")
   set(DOXY_DIR "${DOCUMENTATION_DIR}")
   set(DOXY_CONFIG "${DOXY_DIR}/Doxyfile.in")

   # Copy doxy.config.in
   configure_file("${DOXY_CONFIG}" "${CMAKE_BINARY_DIR}/doxy.config")

   # Create doc directory
   add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/doc
   COMMAND rm -rf ${CMAKE_BINARY_DIR}/doc
   COMMAND mkdir ${CMAKE_BINARY_DIR}/doc
   )

   # Run doxygen
   add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/doc/html/index.html
   COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/doxy.config"
   DEPENDS "${CMAKE_BINARY_DIR}/doxy.config" "${CMAKE_BINARY_DIR}/doc"
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
   )

   add_custom_target(docs ALL DEPENDS ${CMAKE_BINARY_DIR}/doc/html/index.html)

   message(STATUS "Generating API documentation with Doxygen")
else(DOCUMENTATION AND DOXYGEN_FOUND)
   message(STATUS "Not generating API documentation")
endif(DOCUMENTATION AND DOXYGEN_FOUND)
