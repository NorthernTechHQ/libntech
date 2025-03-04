#!/bin/bash

set -x

n_procs="$(getconf _NPROCESSORS_ONLN)"

function check_with_gcc() {
  rm -f config.cache
  make clean
  ./autogen.sh --config-cache --enable-debug CC=gcc
  local gcc_exceptions="-Wno-sign-compare -Wno-enum-int-mismatch"
  make -j -l${n_procs} --keep-going CFLAGS="-Werror -Wall -Wextra $gcc_exceptions"
}

function check_with_clang() {
  rm -f config.cache
  make clean
  ./autogen.sh --config-cache --enable-debug CC=clang
  make -j -l${n_procs} --keep-going CFLAGS="-Werror -Wall -Wextra -Wno-sign-compare"
}

function check_with_cppcheck() {
  rm -f config.cache
  make clean
  ./autogen.sh --config-cache --enable-debug

  # print out cppcheck version for comparisons over time in case of regressions due to newer versions
  cppcheck --version

  # cppcheck options:
  #   -I -- include paths
  #   -i -- ignored files/folders
  #   --include=<file> -- force including a file, e.g. config.h
  # Identified issues are printed to stderr
  cppcheck --quiet -j${n_procs} --error-exitcode=1 ./ \
           --suppressions-list=tests/static-check/cppcheck_suppressions.txt \
           --check-level=exhaustive \
           --include=config.h \
           -I libutils/ \
           -i tests \
           -i libcompat/ \
           2>&1 1>/dev/null
}

cd "$(dirname $0)"/../../

failure=0
failures=""
check_with_gcc              || { failures="${failures}FAIL: GCC check failed\n"; failure=1; }
check_with_clang            || { failures="${failures}FAIL: Clang check failed\n"; failure=1; }
check_with_cppcheck         || { failures="${failures}FAIL: cppcheck failed\n"; failure=1; }

echo -en "$failures"
exit $failure
