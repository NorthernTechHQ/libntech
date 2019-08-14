#!/bin/sh
set -e
set -x

cd $TRAVIS_BUILD_DIR || exit 1

# Unshallow the clone. Fetch the tags from upstream even if we are on a
# foreign clone. Needed for determine-version.py to work, specifically
# `git describe --tags HEAD` was failing once the last tagged commit
# became too old.
git fetch --unshallow
git remote add upstream https://github.com/cfengine/core.git  \
    && git fetch upstream 'refs/tags/*:refs/tags/*'

if [ "$TRAVIS_OS_NAME" = osx ]
then
    ./autogen.sh --enable-debug
    gmake --version
    gmake CFLAGS="-Werror -Wall -Wno-pointer-sign"
    gmake --debug -C tests/unit check
    exit
else
    NO_CONFIGURE=1 ./autogen.sh
    ./configure --enable-debug --prefix=$INSTDIR \
        `[ "x$COVERAGE" != xno ] && echo --enable-coverage`
fi

if [ "$JOB_TYPE" = compile_only ]
then
    make CFLAGS="-Werror -Wno-pointer-sign" -k
elif [ "$JOB_TYPE" = compile_and_unit_test ]
then
    make CFLAGS="-Wall -Wextra -Werror -Wno-pointer-sign -Wno-sign-compare"
    make -C tests/unit check
    exit
elif [ "$JOB_TYPE" = compile_and_unit_test_asan ]
then
    make CFLAGS="-Werror -Wall -Wno-pointer-sign -fsanitize=address" LDFLAGS="-fsanitize=address -pthread"
    make -C tests/unit CFLAGS="-fsanitize=address" LDFLAGS="-fsanitize=address -pthread" check
    exit
else
    make
fi
