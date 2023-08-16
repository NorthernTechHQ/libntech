#!/bin/sh
#
#  Copyright 2021 Northern.tech AS
#
#  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# To the extent this program is licensed as part of the Enterprise
# versions of CFEngine, the applicable Commercial Open Source License
# (COSL) may apply to this file if you as a licensee so wish it. See
# included file COSL.txt.
#
set -e

get_version()
{
  CONF_VERSION=$(sed -ne 's/AM_INIT_AUTOMAKE(cfengine, \(.*\)).*/\1/p' configure.ac)
  case "$CONF_VERSION" in
    *revision*)
      REVISION=$(git rev-list -1 --abbrev-commit HEAD || echo unknown)
      VERSION=$(echo $CONF_VERSION | sed -e "s/revision/${REVISION}/g")
      ;;
    *)
      VERSION=$CONF_VERSION
      ;;
  esac
echo $VERSION
}
dist()

{
  git checkout $BRANCH
  ./autogen.sh
  make dist
}

check()
{
  CURR_VERSION=$(get_version)

  cd ..
  tar xf core/cfengine-$CURR_VERSION.tar.gz
  cd cfengine-$CURR_VERSION
  ./configure --disable-coverage --disable-shared
  make check -j8
}

if [ $# -eq 0 ]; then
  BRANCH=master
  echo
  echo "Branch/tag has not been specified. Using master branch by default."
  echo
else
  BRANCH=$1
  echo
  echo "Using $BRANCH"
  echo
fi

dist $BRANCH
check
