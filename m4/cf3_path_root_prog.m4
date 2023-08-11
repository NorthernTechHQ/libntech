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
# CF3_PATH_ROOT_PROG(variable, program, value-if-not-found, path = $PATH)
# --------------------------------------
#
# This function has almost the same semantics as the AC_PATH_PROG
# function. The difference is that this will detect tools that are
# runnable by root, but not by the current user. These tools are
# typically used not by the build, but by CFEngine itself, after
# it is installed.
#
AC_DEFUN([CF3_PATH_ROOT_PROG],
[
  found=0
  AS_IF([test "x$4" = "x"], [
    path=$PATH
  ], [
    path=$4
  ])
  AS_ECHO_N(["checking for $2... "])
  for i in $(echo $path | sed -e 's/:/ /g'); do
    AS_IF([test -e $i/$2 && ls -ld $i/$2 | grep ['^[^ ][^ ][^ ][xs][^ ][^ ][^ ][^ ][^ ][^ ]'] > /dev/null], [
      $1=$i/$2
      found=1
      break
    ])
  done

  AS_IF([test "$found" = "1"], [
    AS_ECHO(["$][$1"])
  ], [
    AS_ECHO([no])
    $1=$3
  ])
])
