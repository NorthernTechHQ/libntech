# 3.24 introduced some extra find_package() features, but we can easily go lower
# instead of starting using them, this is an arbitrary value for now, but we
# need something otherwise CMake would complain.
cmake_minimum_required(VERSION 3.24)

project(libntech)
add_library(ntech) # so that we get libntech.a

# set(CMAKE_VERBOSE_MAKEFILE ON)

# TODO: support COMPONENTS specified by find_package()
#       https://stackoverflow.com/questions/54702582/how-to-configure-project-with-components-in-cmake
#       https://github.com/markeastwood82/nomnoms/blob/master/noms/fruit/CMakeLists.txt
option(LIBNTECH_SEQUENCE "Include support for the sequence data structure" OFF)
option(LIBNTECH_JSON "Include JSON support" OFF)

option(LIBNTECH_EXAMPLES "Build examples" ON)

get_filename_component(LIBUTILS_DIR ${CMAKE_CURRENT_LIST_DIR}/libutils ABSOLUTE)
get_filename_component(LIBCOMPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/libcompat ABSOLUTE)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/availability_checks.txt")

# File created by autotools which we don't need, but it has to exist.
file(TOUCH "${CMAKE_CURRENT_LIST_DIR}/config.post.h")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
  set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-flto")
  set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-flto -s")
endif()

# TODO: string_lib, logging and even writer as separate components
if(${LIBNTECH_JSON})
  list(APPEND LIBNTECH_SOURCES
    "${LIBUTILS_DIR}/json.c" # main source
    "${LIBUTILS_DIR}/logging.c" "${LIBUTILS_DIR}/misc_lib.c" "${LIBUTILS_DIR}/string_lib.c" "${LIBUTILS_DIR}/writer.c" # dependencies
  )
  # JSON support requires the sequence type
  set(LIBNTECH_SEQUENCE ON)
endif()

if(${LIBNTECH_SEQUENCE})
  list(APPEND LIBNTECH_SOURCES
    "${LIBUTILS_DIR}/sequence.c" # main source
    "${LIBUTILS_DIR}/alloc.c" "${LIBUTILS_DIR}/cleanup.c" "${LIBCOMPAT_DIR}/memdup.c" # dependencies
  )
endif()

target_sources(ntech PRIVATE ${LIBNTECH_SOURCES})
target_include_directories(ntech PUBLIC "${LIBUTILS_DIR}" "${CMAKE_CURRENT_LIST_DIR}")

if(${LIBNTECH_EXAMPLES})
  if(${LIBNTECH_SEQUENCE})
    add_executable(sequence_example "${CMAKE_CURRENT_SOURCE_DIR}/examples/sequence_example.c")
    target_link_libraries(sequence_example ntech)
    target_include_directories(sequence_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(${LIBNTECH_JSON})
    add_executable(json_example "${CMAKE_CURRENT_SOURCE_DIR}/examples/json_example.c")
    target_link_libraries(json_example ntech)
    target_include_directories(json_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endif()
