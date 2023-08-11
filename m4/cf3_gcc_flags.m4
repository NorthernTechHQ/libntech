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
dnl ####################################################################
dnl Set GCC CFLAGS only if using GCC.
dnl ####################################################################

AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
#if defined __HP_cc
#This is HP-UX ANSI C
#endif
]])], [
HP_UX_AC="no"], [
CFLAGS="$CFLAGS -Agcc"
CPPFLAGS="$CPPFLAGS -Agcc"
HP_UX_AC="yes"])

AC_MSG_CHECKING(for HP-UX aC)
if test "x$HP_UX_AC" = "xyes"; then
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING(for GCC specific compile flags)
if test x"$GCC" = "xyes" && test x"$HP_UX_AC" != x"yes"; then
    CFLAGS="$CFLAGS -g -Wall"
    CPPFLAGS="$CPPFLAGS -std=gnu99"
    AC_MSG_RESULT(yes)

    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -Wno-pointer-sign"
    AC_MSG_CHECKING(for -Wno-pointer-sign)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() {}])],
     [AC_MSG_RESULT(yes)],
     [AC_MSG_RESULT(no)
     CFLAGS="$save_CFLAGS"])

    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -Werror=implicit-function-declaration"
    AC_MSG_CHECKING(for -Werror=implicit-function-declaration)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() {}])],
     [AC_MSG_RESULT(yes)],
     [AC_MSG_RESULT(no)
     CFLAGS="$save_CFLAGS"])

    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -Wunused-parameter"
    AC_MSG_CHECKING(for -Wunused-parameter)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() {}])],
     [AC_MSG_RESULT(yes)],
     [AC_MSG_RESULT(no)
     CFLAGS="$save_CFLAGS"])

    dnl Clang does not like 'const const' construct arising from
    dnl expansion of TYPED_SET_DECLARE macro
    dnl
    dnl This check is relying on explicit compilator detection due to
    dnl GCC irregularities checking for -Wno-* command-line options
    dnl (command line is not fully checked until actual warning occurs)
    AC_MSG_CHECKING(for -Wno-duplicate-decl-specifier)
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([#ifndef __clang__
# error Not a clang
#endif
int main() {}])],
     [AC_MSG_RESULT(yes)
     CFLAGS="$save_CFLAGS -Wno-duplicate-decl-specifier"],
     [AC_MSG_RESULT(no)])
else 
    AC_MSG_RESULT(no)
fi
