#!/bin/sh
set -e
if [ "$TRAVIS_OS_NAME" = osx ]
then
    set +e
    rvm get stable
    brew update
    brew install lmdb
    brew --prefix lmdb
    brew install make
    brew --prefix make
    brew install autoconf
    brew --prefix autoconf
    brew install automake
    brew --prefix automake
    brew install openssl
    brew --prefix openssl
    # brew install gcc@7 || brew link --overwrite gcc@7
    set -e
    # gcc-7 --version
    #brew install python
    #brew install libxml2
    #brew install fakeroot
else
    sudo rm -vf /etc/apt/sources.list.d/*riak*
    sudo apt-get --quiet update
    # Needed to build
    sudo apt-get install -y libpam0g-dev
    sudo apt-get install -y liblmdb-dev
    # Needed to test
    sudo apt-get install -y fakeroot

    if [ "$JOB_TYPE" != compile_and_unit_test_no_deps ]
    then
        # Optional dependencies, don't install in no_deps job
        sudo apt-get install -y libssl-dev
        sudo apt-get install -y libxml2-dev libacl1-dev
    fi
    # codecov.io dependency
    sudo apt-get install -y lcov
    # Ensure traditional yacc compatibility
    sudo apt-get purge      -y bison
    sudo apt-get autoremove -y
    sudo apt-get install    -y byacc
    sudo apt-get -qy install curl
fi
