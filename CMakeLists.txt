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

get_filename_component(LIBUTILS_DIR ${CMAKE_CURRENT_LIST_DIR}/libutils ABSOLUTE)
get_filename_component(LIBCOMPAT_DIR ${CMAKE_CURRENT_LIST_DIR}/libcompat ABSOLUTE)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/availability_checks.txt")

# File created by autotools which we don't need, but it has to exist.
file(TOUCH "${CMAKE_CURRENT_LIST_DIR}/config.post.h")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

if(${LIBNTECH_SEQUENCE})
  list(APPEND LIBNTECH_SOURCES
    "${LIBUTILS_DIR}/sequence.c" # main source
    "${LIBUTILS_DIR}/alloc.c" "${LIBUTILS_DIR}/cleanup.c" "${LIBCOMPAT_DIR}/memdup.c" # dependencies
  )
endif()

target_sources(ntech PRIVATE ${LIBNTECH_SOURCES})
target_include_directories(ntech PUBLIC "${LIBUTILS_DIR}" "${CMAKE_CURRENT_LIST_DIR}")

if(${LIBNTECH_SEQUENCE})
  add_executable(sequence_example "${CMAKE_CURRENT_SOURCE_DIR}/examples/sequence_example.c")
  target_link_libraries(sequence_example ntech)
  target_include_directories(sequence_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
