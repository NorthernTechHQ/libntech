dnl
dnl  Copyright 2024 Northern.tech AS
dnl
dnl  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.
dnl
dnl  Licensed under the Apache License, Version 2.0 (the "License");
dnl  you may not use this file except in compliance with the License.
dnl  You may obtain a copy of the License at
dnl
dnl      http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl  Unless required by applicable law or agreed to in writing, software
dnl  distributed under the License is distributed on an "AS IS" BASIS,
dnl  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl  See the License for the specific language governing permissions and
dnl  limitations under the License.
dnl
dnl To the extent this program is licensed as part of the Enterprise
dnl versions of CFEngine, the applicable Commercial Open Source License
dnl (COSL) may apply to this file if you as a licensee so wish it. See
dnl included file COSL.txt.
dnl
dnl ##########################################################################
dnl #
dnl # Build libntech
dnl #
dnl # Run ./autogen.sh to build configure script
dnl #
dnl ##########################################################################

AC_PREREQ(2.63)

AC_INIT(libntech, 1.0.0, [], [], [https://github.com/NorthernTechHQ/libntech])
AC_CANONICAL_TARGET

dnl
dnl This program needs to be checked early, as MAKEINFO variable is expanded in
dnl AM_INIT_AUTOMAKE.
dnl
AC_CHECK_PROG(MAKEINFO, makeinfo, makeinfo)


dnl Parallel unit tests are causing spurious failures across several systems,
dnl particularly those doing process testing.
dnl Unfortunately the option to disable parallel tests (serial-tests) doesn't
dnl exist in automake 1.11 and earlier, so we need to do this complicated logic
dnl to determine whether we can disable it or not. If it doesn't exist as an
dnl option, then serial tests are already the default.
AC_MSG_CHECKING([automake version])
m4_define(AUTOMAKE_VERSION, m4_normalize(m4_esyscmd([automake --version 2>&1 | sed -ne '/^automake/{s/^automake.* \([^ ][^ ]*\)$/\1/; p;}'])))
m4_define(SERIAL_TESTS, m4_bmatch(AUTOMAKE_VERSION, [^1\.\([0-9]\|1[0-1]\)\(\.\|$\)], [], [serial-tests]))
AC_MSG_RESULT(AUTOMAKE_VERSION)


AM_INIT_AUTOMAKE([tar-ustar] SERIAL_TESTS)
AM_MAINTAINER_MODE([enable])

m4_divert_text([DEFAULTS], [: "${AR_FLAGS=cr}"])

AC_DEFINE(BUILD_YEAR, esyscmd([date +%Y | tr -d '\n']), "Software build year")

AC_DEFINE_UNQUOTED(ABS_TOP_SRCDIR,
"`cd -- "$srcdir"; pwd`",
[Absolute path of source tree])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

dnl Libtool madness

AC_CONFIG_MACRO_DIR([m4])

dnl
dnl hide [s]include macros, so old aclocal (automake < 1.10) won't find them and
dnl won't complain about something/something.m4 not found
dnl
m4_define(incstart,sinc)
m4_define(incend,lude)

dnl
dnl Save environment CFLAGS before autoconf starts messing with it.
dnl It is restored later.
dnl
ENV_CFLAGS="$CFLAGS"

dnl ######################################################################
dnl Checks for programs.
dnl ######################################################################

AC_PROG_CC
AC_PROG_MKDIR_P
AC_EXEEXT

dnl GCC specific flags
m4_include([m4/cf3_gcc_flags.m4])

# Use either new LT_INIT or old AC_DISABLE_STATIC/AC_PROG_LIBTOOL macros
m4_ifdef([LT_INIT],
  [LT_INIT([disable-static])],
  [AC_DISABLE_STATIC
   AC_PROG_LIBTOOL])

AM_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_PATH_PROG([PERL], [perl])
AC_CONFIG_LIBOBJ_DIR(libcompat)
AC_PATH_PROG(GETCONF, getconf, false, $PATH:$prefix/bin:/usr/bin:/usr/local/bin:/sw/bin)
AM_CONDITIONAL(CROSS_COMPILING, test "x$cross_compiling" = "xyes")

# Check whether `pkg-config' is available
AC_ARG_VAR([PKG_CONFIG], [path to pkg-config])
AC_ARG_VAR([PKG_CONFIG_PATH], [directories to add to the pkg-config search path])
AC_ARG_VAR([PKG_CONFIG_LIBDIR], [path overriding pkg-config's search path])

if test "x$ac_cv_env_PKG_CONFIG_set" != "xset"; then
    AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
fi

dnl ######################################################################
dnl Use pthreads if available
dnl ######################################################################

AC_ARG_WITH([pthreads],
    [AS_HELP_STRING([--with-pthreads[[=PATH]]],
                    [Specify path to pthreads, if not the part of operating system])])

if test "x$with_pthreads" != x && test "x$with_pthreads" != "xyes" && test "x$with_pthreads" != "xno"; then
   LIBS="$LIBS -L$with_pthreads/lib"
   CPPFLAGS="-I$with_pthreads/include $CPPFLAGS"
fi

ACX_PTHREAD([],
            [AC_MSG_ERROR(pthread-compatible library is required to build CFEngine)])

CC="$PTHREAD_CC"
CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
LIBS="$PTHREAD_LIBS $LIBS"

dnl ######################################################################
dnl Configure files layout
dnl ######################################################################

AC_ARG_ENABLE([fhs],
        [AS_HELP_STRING([--enable-fhs], [Enable FHS compliance. Defaults to custom CFEngine files layout])])

#
# pkglibdir/pkgdatadir are not overridable, so use our own invention instead.
#

AS_IF([test x"$enable_fhs" = xyes], [
  projlibdir='${pkglibdir}'
  WORKDIR='${localstatedir}/lib/cfengine'
  MASTERDIR='default'
  INPUTDIR='default'
  DATADIR='default'
  LOGDIR='${localstatedir}/log/cfengine'
  PIDDIR='${runstatedir:-${localstatedir}/run}/cfengine'
  STATEDIR='default'
], [
  if test x"$prefix" = xNONE || test x"$prefix" = x/var/cfengine; then
    prefix=/var/cfengine
    case "$target_os" in
      mingw*)
        WORKDIR=$(cmd /c "echo %PROGRAMFILES%\\Cfengine" | sed 's/\\/\\\\/g')
        MASTERDIR=default
        INPUTDIR=default
        DATADIR=default
        LOGDIR=$(cmd /c "echo %PROGRAMFILES%\\Cfengine" | sed 's/\\/\\\\/g')
        PIDDIR=$(cmd /c "echo %PROGRAMFILES%\\Cfengine" | sed 's/\\/\\\\/g')
        STATEDIR=default
      ;;
      *)
        WORKDIR=/var/cfengine
        MASTERDIR=default
        INPUTDIR=default
        DATADIR=default
        LOGDIR=/var/cfengine
        PIDDIR=/var/cfengine
        STATEDIR=default
      ;;
    esac
  else
    WORKDIR="${localstatedir}/${PACKAGE}"
    MASTERDIR="default"
    INPUTDIR="default"
    DATADIR="default"
    LOGDIR="${localstatedir}/${PACKAGE}"
    PIDDIR="${localstatedir}/${PACKAGE}"
    STATEDIR="default"
  fi
  bindir="${bindir:-${exec_prefix}/bin}"
  projlibdir='${libdir}'
])

AC_SUBST(projlibdir)

dnl ######################################################################
dnl Platform specific compiler flags.
dnl ######################################################################

AS_CASE([${target_os}],
  [mingw*],
    # Disable printf format warnings, because our wrapper supports more
    # flags than vanilla Windows version, so they are false positives.
    [CFLAGS="$CFLAGS -Wno-format"])

dnl ######################################################################
dnl Enable debugging
dnl ######################################################################

AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug], [Enable debugging]),
              [debug=$enableval],
              [debug=no])
AM_CONDITIONAL([NDEBUG], [test x"$debug" = x"no"])

dnl Even though CFLAGS should contain the command-line CFLAGS
dnl as last, some macro seem to messes the order up and insert
dnl its own optimisation flags as well. So we append ENV_CFLAGS
dnl at the end manually, causing a bit of flag duplication.

AC_MSG_CHECKING([for debug option])
if test x"$debug" = x"yes"
then
    AC_MSG_RESULT(yes)
    CFLAGS="$CFLAGS -g3 -O0 $ENV_CFLAGS"
else
    AC_MSG_RESULT(no)
    CFLAGS="$CFLAGS -O2 -DNDEBUG $ENV_CFLAGS"
fi

dnl ######################################################################
dnl Checks for libraries.
dnl ######################################################################

dnl Now check for database connectors

dnl
dnl OpenSSL
dnl

AC_ARG_WITH(openssl,
    [AS_HELP_STRING([--with-openssl[[=PATH]]],
    [Specify OpenSSL path])], [], [with_openssl=yes])

if  test -d /usr/local/Cellar/ && \
    test -d /usr/local/opt/openssl/ && \
    test "x$with_openssl" = "xyes" ; then
    with_openssl=$(brew --prefix openssl)
    echo "OS X Homebrew detected"
    echo "Defaulting to: --with-openssl=$with_openssl"
fi

if test "x$with_openssl" != "xno"; then
  CF3_WITH_LIBRARY(openssl, [
    AC_CHECK_LIB(crypto, RSA_generate_key_ex, [], [])
    AC_CHECK_LIB(ssl, SSL_free, [], [])
    AC_CHECK_DECLS([SSL_CTX_clear_options], [], [], [[#include <openssl/ssl.h>]])
    AC_CHECK_HEADERS([openssl/opensslv.h], [], [AC_MSG_ERROR(Cannot find OpenSSL)])
    AC_MSG_CHECKING(for OpenSSL version)
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[
    #include <openssl/opensslv.h>

    #if OPENSSL_VERSION_NUMBER < 0x1000000fL
    #This OpenSSL is too old
    #endif
    ]])],[AC_MSG_RESULT(OK)],[AC_MSG_ERROR(This release of CFEngine requires OpenSSL >= 1.0.0)])
    if test "x$ac_cv_lib_crypto_RSA_generate_key_ex" = "xno"; then
       AC_MSG_ERROR(Cannot find OpenSSL)
    fi

    AC_DEFINE([OPENSSL_SUPPRESS_DEPRECATED], [1], [Suppress deprecation warnings from OpenSSL 3])

    AC_CHECK_DECL([SSL_OP_NO_TLSv1_1],
     [AC_DEFINE([HAVE_TLS_1_1], [1], [Define if TLS 1.1 is supported by OpenSSL])],
     [], [[#include <openssl/ssl.h>]]
     )
    AC_CHECK_DECL([SSL_OP_NO_TLSv1_2],
     [AC_DEFINE([HAVE_TLS_1_2], [1], [Define if TLS 1.2 is supported by OpenSSL])],
     [], [[#include <openssl/ssl.h>]]
     )
    AC_CHECK_DECL([SSL_OP_NO_TLSv1_3],
     [AC_DEFINE([HAVE_TLS_1_3], [1], [Define if TLS 1.3 is supported by OpenSSL])],
     [], [[#include <openssl/ssl.h>]]
     )
  ])
  AC_DEFINE(WITH_OPENSSL, 1, [Define if OpenSSL is being used])
  AM_CONDITIONAL(WITH_OPENSSL, true)
else
  AM_CONDITIONAL(WITH_OPENSSL, false)
fi

dnl PCRE2
AC_ARG_WITH([pcre], [AS_HELP_STRING([--with-pcre=no], [Disable PCRE (only for backwards compatibility)])], [], [with_pcre=default])
if test "x$with_pcre" != "xdefault" && test "x$with_pcre" != "xno"; then
  AC_MSG_ERROR([PCRE no longer supported, use PCRE2]);
fi

AC_ARG_WITH([pcre2], [AS_HELP_STRING([--with-pcre2[[=PATH]]], [Specify PCRE2 path])], [], [with_pcre2=yes])

if test "x$with_pcre" != "xno" && test "x$with_pcre2" != "xno"; then
  CF3_WITH_LIBRARY(pcre2, [
    AC_CHECK_LIB(pcre2-8, pcre2_compile_8, [], [AC_MSG_ERROR(Cannot find PCRE2)])
    AC_CHECK_HEADERS(
      [pcre2.h],
      [],
      AC_MSG_ERROR(Cannot find PCRE2),
      [#define PCRE2_CODE_UNIT_WIDTH 8]
      )]
  )
  AC_DEFINE(WITH_PCRE2, 1, [Define if PCRE2 is being used])
  AM_CONDITIONAL(WITH_PCRE2, true)
  WITH_PCRE2_DEFINE="#define WITH_PCRE2 1"
else
  AM_CONDITIONAL(WITH_PCRE2, false)
  WITH_PCRE2_DEFINE="// #undef WITH_PCRE2"
fi
AC_SUBST(WITH_PCRE2_DEFINE)

dnl systemd structured logging

AC_ARG_WITH([systemd-logging], [AS_HELP_STRING([--with-systemd-logging[[=PATH]]], [support systemd structured logging])], [], [with_systemd_logging=check])

if test "x$with_systemd_logging" != xno
then
   CF3_WITH_LIBRARY(systemd_logging, [
      AC_CHECK_LIB(systemd, sd_journal_sendv, [], [if test "x$with_systemd_logging" != xcheck; then AC_MSG_ERROR(Cannot find systemd library); fi])
      AC_CHECK_HEADERS(systemd/sd-journal.h, [], [if test "x$with_systemd_logging" != xcheck; then AC_MSG_ERROR(Cannot find systemd headers); fi])
   ])
fi

dnl libyaml

AC_ARG_WITH([libyaml],
    [AS_HELP_STRING([--with-libyaml[[=PATH]]], [Specify libyaml path])], [], [with_libyaml=check])

if test "x$with_libyaml" != xno
then
  CF3_WITH_LIBRARY(libyaml, [
    AC_CHECK_LIB(yaml, yaml_parser_initialize,
      [],
      [if test "x$with_libyaml" != xcheck; then AC_MSG_ERROR(Cannot find libyaml library); fi])
    AC_CHECK_HEADERS(yaml.h,
      [libyaml_header_found=yes
      HAVE_LIBYAML_DEFINE="#define HAVE_LIBYAML 1"],
      [if test "x$with_libyaml" != xcheck; then AC_MSG_ERROR(Cannot find libyaml header files); fi
      HAVE_LIBYAML_DEFINE="// #undef HAVE_LIBYAML"])
  ])
else
  HAVE_LIBYAML_DEFINE="// #undef HAVE_LIBYAML"
fi
AC_SUBST(HAVE_LIBYAML_DEFINE)

dnl ######################################################################
dnl Checks for header files.
dnl ######################################################################

AC_CHECK_HEADERS(unistd.h stdlib.h sys/loadavg.h)
AC_CHECK_HEADERS(sys/param.h sys/resource.h)

# sys/param.h is required for sys/mount.h on OpenBSD
AC_CHECK_HEADERS(sys/mount.h, [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

# Required on BSD to get struct sockaddr_dl (for retrieving MAC addresses from getifaddrs())
AC_CHECK_HEADERS(net/if_dl.h)

# Required on Solaris to get struct arpreq (for retrieving MAC addresses)
AC_CHECK_HEADERS(net/if_arp.h, , , [AC_INCLUDES_DEFAULT
#include <sys/socket.h>
])

AC_CHECK_HEADERS(getopt.h,
    [HAVE_GETOPT_H_DEFINE="#define HAVE_GETOPT_H 1"],
    [HAVE_GETOPT_H_DEFINE="// #undef HAVE_GETOPT_H"]
)
AC_SUBST(HAVE_GETOPT_H_DEFINE)

AC_CHECK_HEADERS(utime.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(malloc.h sys/malloc.h)
AC_CHECK_HEADERS(vfs.h)
AC_CHECK_HEADERS(sys/vfs.h)
AC_CHECK_HEADERS(sys/sockio.h)
AC_CHECK_HEADERS(sys/statvfs.h)
AC_CHECK_HEADERS(sys/statfs.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(sys/filesys.h)
AC_CHECK_HEADERS(dustat.h)
AC_CHECK_HEADERS(sys/systeminfo.h)
AC_CHECK_HEADERS(ieeefp.h)
AC_CHECK_HEADERS(winsock2.h)
AC_CHECK_HEADERS(ws2tcpip.h)
AC_CHECK_HEADERS(zone.h)
AC_CHECK_HEADERS(sys/uio.h)
AC_CHECK_HEADERS_ONCE([sys/sysmacros.h]) dnl glibc deprecated inclusion in sys/type.h
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(sys/mpctl.h) dnl For HP-UX $(sys.cpus) - Mantis #1069
AC_CHECK_HEADERS(shadow.h)
AC_CHECK_HEADERS(sys/jail.h, [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])
AC_CHECK_HEADERS(net/route.h netinet/in.h netinet/ip.h)


AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_HEADER_DIRENT
AC_HEADER_STDBOOL

dnl ######################################################################
dnl Checks for data types
dnl ######################################################################

AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_CHECK_TYPES(clockid_t,
    [HAVE_CLOCKID_T_DEFINE="#define HAVE_CLOCKID_T 1"],
    [HAVE_CLOCKID_T_DEFINE="// #undef HAVE_CLOCKID_T"]
, [[
#ifdef HAVE_TIME_H
# include <time.h>
#endif
]])
AC_SUBST(HAVE_CLOCKID_T_DEFINE)

AC_CHECK_TYPES(socklen_t, [], [], [[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <sys/socket.h>
]])

dnl ######################################################################
dnl Checks for typedefs, structures, and compiler characteristics.
dnl ######################################################################

AC_C_CONST

AC_FUNC_FSEEKO
AC_SYS_LARGEFILE
AC_TYPE_OFF_T

#
# AC_SYS_LARGEFILE correctly figures out necessary macros for large files, but
# on AIX there is a gotcha:
#
# Code generated by flex #includes <stdio.h> at the beginning of the file, which
# picks up 32-bit wide off_t. Then it #includes <config.h> which provides LFS
# macros, and finally it includes another system header, now with 64-bit wide
# off_t, which causes a conflict.
#
if test "x$ac_cv_sys_large_files" = x1; then
  CPPFLAGS="-D_LARGE_FILES=1 $CPPFLAGS"
fi

dnl ######################################################################
dnl Check for libraries
dnl ######################################################################

AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(rt, clock_gettime)
AC_CHECK_LIB(dl, dlopen)

dnl ######################################################################
dnl Check for special functions
dnl ######################################################################

AC_CHECK_DECLS(clock_gettime, [], [], [[#include <time.h>]])
AC_REPLACE_FUNCS(clock_gettime)

AC_CHECK_DECLS(unsetenv)
AC_REPLACE_FUNCS(unsetenv)

AC_CHECK_DECLS(strnlen)
AC_REPLACE_FUNCS(strnlen)

cf3_FUNC_STRNDUP

AC_CHECK_DECLS(seteuid)
AC_REPLACE_FUNCS(seteuid)

AC_CHECK_DECLS(setlinebuf)
AC_REPLACE_FUNCS(setlinebuf)

AC_CHECK_DECLS(strlcat)
AC_REPLACE_FUNCS(strlcat)

AC_CHECK_DECLS(strlcpy)
AC_REPLACE_FUNCS(strlcpy)

AC_CHECK_DECLS(realpath)

AC_CHECK_DECLS(strdup)
AC_REPLACE_FUNCS(strdup)

AC_CHECK_DECLS(memrchr)
AC_REPLACE_FUNCS(memrchr)

AC_CHECK_DECLS(round, [], [], [[#include <math.h>]])
AC_REPLACE_FUNCS(round)

AC_CHECK_DECLS(nanosleep)
AC_REPLACE_FUNCS(nanosleep)

AC_CHECK_DECLS(memdup)
AC_REPLACE_FUNCS(memdup)

AC_CHECK_DECLS(memmem)
AC_REPLACE_FUNCS(memmem)

AC_CHECK_DECLS(srand48)
AC_REPLACE_FUNCS(srand48)

AC_CHECK_DECLS(drand48)
AC_REPLACE_FUNCS(drand48)

AC_CHECK_DECLS(strerror)
AC_REPLACE_FUNCS(strerror)

AC_CHECK_DECLS(strstr)
AC_REPLACE_FUNCS(strstr)

AC_CHECK_DECLS(strcasestr)
AC_REPLACE_FUNCS(strcasestr)

AC_CHECK_DECLS(strcasecmp)
AC_REPLACE_FUNCS(strcasecmp)

AC_CHECK_DECLS(strncasecmp)
AC_REPLACE_FUNCS(strncasecmp)

AC_CHECK_DECLS(strsep)
AC_REPLACE_FUNCS(strsep)

AC_CHECK_DECLS(strsignal)
AC_REPLACE_FUNCS(strsignal)

AC_CHECK_DECLS(gmtime_r, [], [], [[#include <time.h>]])
AC_REPLACE_FUNCS(gmtime_r)

AC_CHECK_DECLS(getline, [], [], [#define _GNU_SOURCE 1
AC_INCLUDES_DEFAULT])
AC_REPLACE_FUNCS(getline)

AC_CHECK_DECLS(strchrnul, [], [], [#define _GNU_SOURCE 1
AC_INCLUDES_DEFAULT])
AC_REPLACE_FUNCS(strchrnul)

AC_CHECK_DECLS(localtime_r, [], [], [[#include <time.h>]])
AC_REPLACE_FUNCS(localtime_r)

AC_CHECK_DECLS(fgetgrent, [], [], [[#include <grp.h>]])

AC_CHECK_DECLS(isfinite, [], [], [[#include <math.h>]])

AC_CHECK_FUNCS(getpwent setpwent endpwent)

AC_CHECK_FUNCS(fgetspent lckpwdf ulckpwdf)
AC_CHECK_LIB([sec], [fgetspent], [
    AC_DEFINE([HAVE_LIBSEC], 1, [Define if -lsec is available])
    AC_DEFINE([HAVE_FGETSPENT], 1, [Define if fgetspent is available])
    LIBS="-lsec $LIBS"
])

AC_C_BIGENDIAN
AC_CHECK_HEADERS([endian.h])
AC_CHECK_DECLS(le32toh, [], [], [[#include <endian.h>]])

AC_CHECK_DECLS(closefrom, [], [], [[#include <unistd.h>
                                    #include <stdlib.h>]])
AC_REPLACE_FUNCS(closefrom)

AC_CHECK_HEADERS([sys/pstat.h])
AC_CHECK_FUNCS(pstat_getfile2)

CF3_PATH_ROOT_PROG([CHPASSWD], [chpasswd], [], [/sbin:/usr/sbin:/bin:/usr/bin:$PATH])
AS_IF([test "x$CHPASSWD" != "x"],
      [AC_DEFINE(HAVE_CHPASSWD, 1, [Define if chpasswd tool is present])]
      [AC_DEFINE_UNQUOTED(CHPASSWD, ["$CHPASSWD"], [Path to chpasswd tool])]
     )

dnl AIX has this.
CF3_PATH_ROOT_PROG([PWDADM], [pwdadm], [], [/sbin:/usr/sbin:/bin:/usr/bin:$PATH])
AS_IF([test "x$PWDADM" != "x"],
      [AC_DEFINE(HAVE_PWDADM, 1, [Define if pwdadm tool is present])]
      [AC_DEFINE_UNQUOTED(PWDADM, ["$PWDADM"], [Path to pwdadm tool])]
     )

CF3_PATH_ROOT_PROG([USERADD], [useradd], [], [/sbin:/usr/sbin:/bin:/usr/bin:$PATH])
AS_IF([test "x$USERADD" != "x"],
      [AC_DEFINE(HAVE_USERADD, 1, [Define if useradd tool is present])]
      [AC_DEFINE_UNQUOTED(USERADD, ["$USERADD"], [Path to useradd tool])]
     )

CF3_PATH_ROOT_PROG([USERMOD], [usermod], [], [/sbin:/usr/sbin:/bin:/usr/bin:$PATH])
AS_IF([test "x$USERMOD" != "x"],
      [AC_DEFINE(HAVE_USERMOD, 1, [Define if usermod tool is present])]
      [AC_DEFINE_UNQUOTED(USERMOD, ["$USERMOD"], [Path to usermod tool])]
     )

CF3_PATH_ROOT_PROG([USERDEL], [userdel], [], [/sbin:/usr/sbin:/bin:/usr/bin:$PATH])
AS_IF([test "x$USERDEL" != "x"],
      [AC_DEFINE(HAVE_USERDEL, 1, [Define if userdel tool is present])]
      [AC_DEFINE_UNQUOTED(USERDEL, ["$USERDEL"], [Path to userdel tool])]
     )

AS_IF([test "x$USERADD" != x && \
       test "x$USERMOD" != x && test "x$USERDEL" != x],
      [have_userprogs=yes],
      [have_userprogs=no]
)

AC_CHECK_DECLS(getnetgrent, [], [], [[#include <netdb.h>]])
AC_CHECK_FUNCS(getnetgrent)

AC_CHECK_DECLS(setnetgrent, [], [], [[#include <netdb.h>]])
AC_CHECK_FUNCS(setnetgrent)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]],
                                   [[extern int setnetgrent(const char *)]])],
  [AC_DEFINE([SETNETGRENT_RETURNS_INT], 1, [Whether setnetgrent returns int])],
  [AC_DEFINE([SETNETGRENT_RETURNS_INT], 0, [Whether setnetgrent returns int])])


AC_CHECK_DECLS(endnetgrent, [], [], [[#include <netdb.h>]])
AC_CHECK_FUNCS(endnetgrent)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]],
                                   [[extern int endnetgrent(const char *)]])],
  [AC_DEFINE([ENDNETGRENT_RETURNS_INT], 1, [Whether endnetgrent returns int])],
  [AC_DEFINE([ENDNETGRENT_RETURNS_INT], 0, [Whether endnetgrent returns int])])


AC_CHECK_FUNCS(sendto)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
                                     #include <sys/socket.h>]],
                                   [[extern ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)]])],
    [AC_DEFINE([SENDTO_RETURNS_SSIZE_T], 1, [Whether sendto returns ssize_t])],
    [AC_DEFINE([SENDTO_RETURNS_SSIZE_T], 0, [Whether sendto does not returns ssize_t])])

CF3_CHECK_PROPER_FUNC([ctime],
  [],
  [[#error ctime(3) may produce different results on different OSes. Let's have our POSIX-compliant implementation all the time]],
  [#define ctime rpl_ctime])
CF3_REPLACE_PROPER_FUNC([ctime])

dnl Check whether mkdir accepts two parameters
CF3_CHECK_PROPER_FUNC([mkdir],
  [[#include <sys/stat.h>
    #include <sys/types.h>]],
  [[int mkdir(const char *pathname, mode_t mode);]],
  [#define mkdir rpl_mkdir])

dnl Check for Win32 stat. We don't know to detect improper stat during
dnl compilation, so we resort to OS type detection.

CF3_CHECK_PROPER_FUNC([stat],
  [[#include <sys/types.h>
    #include <sys/stat.h>]],
  [[#if defined(__MINGW32__)
    #error stat in Windows CRT ill-behaves
    #endif]],
  [])

dnl Check for Win32 rename. We don't know how to detect improper rename (not
dnl removing target file if it exists) during cross-compilation, so we resort to
dnl OS type detection.

CF3_CHECK_PROPER_FUNC([rename],
  [],
  [[#if defined(__MINGW32__)
    #error rename in Windows CRT ill-behaves
    #endif]],
  [#define rename rpl_rename])

AC_CHECK_DECLS(mkdtemp)
AC_REPLACE_FUNCS(mkdtemp)

AC_CHECK_DECLS(strrstr)
AC_REPLACE_FUNCS(strrstr)

AC_CHECK_DECLS(stpncpy)
AC_REPLACE_FUNCS(stpncpy)

AC_CHECK_FUNCS(seteuid setegid setreuid setregid)
AC_CHECK_FUNCS(uname gethostname chflags)
AC_CHECK_FUNCS(mkfifo statfs statvfs door)
AC_CHECK_FUNCS(sysinfo setsid sysconf)
AC_CHECK_FUNCS(getzoneid getzonenamebyid)
AC_CHECK_FUNCS(fpathconf)

AC_CHECK_MEMBERS([struct stat.st_mtim, struct stat.st_mtimespec])
AC_CHECK_MEMBERS([struct stat.st_blocks])

AC_MSG_CHECKING([for PRIuMAX/PRIdMAX macros])
AC_EGREP_CPP([primacros_found],
  AC_INCLUDES_DEFAULT
  [#include <inttypes.h>
  #if defined(PRIuMAX) && defined(PRIdMAX)
    primacros_found
  #endif
  ],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   AC_MSG_RESULT(Unable to find out how to scan intmax_t/uintmax_t types)])

HW_FUNC_VSNPRINTF
HW_FUNC_SNPRINTF
HW_FUNC_VASPRINTF
HW_FUNC_ASPRINTF

dnl dirfd might be a function or a macro

AC_CHECK_DECLS(dirfd, [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
])

AC_CHECK_FUNCS(dirfd, [], [
  AC_MSG_CHECKING([for dirfd macro])
  AC_EGREP_CPP([dirfd_found],
    AC_INCLUDES_DEFAULT
    [#ifdef HAVE_DIRENT_H
    # include <dirent.h>
    #endif
    #ifdef dirfd
      dirfd_found
    #endif
    ],
  [AC_MSG_RESULT(yes)
  DIRFD_MACRO_FOUND=1],
  [AC_MSG_RESULT(no)])
  if test x$DIRFD_MACRO_FOUND = x; then
    AC_LIBOBJ([dirfd])
  fi])

AC_CHECK_FUNCS(jail_get)

dnl
dnl Various functions
dnl

AC_SEARCH_LIBS(setsockopt, socket)
AC_SEARCH_LIBS(gethostent, nsl)

AC_CHECK_FUNCS(socket)
AC_CHECK_FUNCS(setsockopt)
AC_CHECK_FUNCS(gethostent)

AC_CHECK_TYPES(struct sockaddr_storage, [], [], [[
#if HAVE_WINSOCK2_H
    #include <winsock2.h>
#endif
#if HAVE_WS2TCPIP_H
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netdb.h>
#endif]])

AC_CHECK_DECLS(getaddrinfo, [], [AC_LIBOBJ(getaddrinfo)], [[
#if HAVE_WINSOCK2_H
    #include <winsock2.h>
#endif
#if HAVE_WS2TCPIP_H
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netdb.h>
#endif
]])

AC_CHECK_DECLS([[inet_ntop], [inet_pton]], [], [], [[#include <arpa/inet.h>]])
AC_REPLACE_FUNCS(inet_ntop inet_pton)

AC_CHECK_FUNCS(getifaddrs)

AC_CHECK_FUNCS(getprocs64)

AC_CHECK_FUNC(lchown, AC_DEFINE(HAVE_LCHOWN, 1, [Whether to use lchown(3) to change ownerships]))

AC_CHECK_DECLS(pthread_attr_setstacksize, [], [], [[#include <pthread.h>]])
AC_REPLACE_FUNCS(pthread_attr_setstacksize)

AC_CHECK_DECLS(pthread_sigmask, [], [], [[#include <signal.h>]])
AC_REPLACE_FUNCS(pthread_sigmask)

AC_CHECK_DECLS(sched_yield, [], [], [[#include <sched.h>]])
AC_CHECK_FUNCS(sched_yield)

AC_CHECK_DECLS([openat], [], [], [[#define _GNU_SOURCE 1
                                   #include <fcntl.h>]])
AC_CHECK_DECLS([fstatat], [], [], [[#define _GNU_SOURCE 1
                                    #include <sys/stat.h>]])
AC_CHECK_DECLS([fchownat], [], [], [[#define _GNU_SOURCE 1
                                     #include <unistd.h>]])
AC_CHECK_DECLS([fchmodat], [], [], [[#define _GNU_SOURCE 1
                                    #include <sys/stat.h>]])
AC_REPLACE_FUNCS(openat fstatat fchownat fchmodat readlinkat)

AC_CHECK_DECLS([log2], [], [], [[#include <math.h>]])
AC_REPLACE_FUNCS(log2)

dnl ######################################################################
dnl Required by cf-upgrade. It cannot be implemented in libcompat because
dnl cf-upgrade does not link to any libraries except libutils and only
dnl statically.
dnl ######################################################################
AC_CHECK_FUNC(fexecve, AC_DEFINE(HAVE_FEXECVE, 1, [Whether to use fexecve(3) to execute a new process]))

dnl ######################################################################
dnl These need declarations here, but will be defined in the
dnl Enterprise Windows code.
dnl ######################################################################

AC_CHECK_DECLS(alarm)
AC_CHECK_DECLS(chmod)
AC_CHECK_DECLS(chown)
AC_CHECK_DECLS(fchmod)
AC_CHECK_FUNCS(fchmod)
AC_CHECK_DECLS(uname)
AC_CHECK_DECLS(getuid)
AC_CHECK_DECLS(getgid)
AC_CHECK_DECLS(lstat)
AC_CHECK_FUNCS(sleep)
AC_CHECK_DECLS(socketpair, [], [], [[#include <sys/socket.h>]])
AC_CHECK_DECLS(fsync)
AC_CHECK_DECLS(glob, [], [], [[#include <glob.h>]])

dnl ######################################################################
dnl Check for sa_len in struct sockaddr
dnl ######################################################################

AC_CHECK_MEMBERS([struct sockaddr.sa_len], , , [
#include <sys/types.h>
#include <sys/socket.h>])

AC_CHECK_MEMBERS([struct ifreq.ifr_hwaddr],,, [
#include <sys/ioctl.h>
#include <net/if.h>])

dnl BSD uses sys/sysctl.h for CPU counting
AC_CHECK_HEADERS(sys/sysctl.h, [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

dnl ######################################################################
dnl Requirements for GetUptimeMinutes() to set $(sys.uptime)
dnl ######################################################################
dnl Linux sysinfo() call
AC_CHECK_MEMBERS([struct sysinfo.uptime], , , [#include <sys/sysinfo.h>])

dnl BSD uses sys/sysctl.h to get time-of-boot
AC_CHECK_HEADERS(sys/sysctl.h, [], [], [AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
])

dnl Solaris uses kstat.h to get time-of-boot
AC_CHECK_HEADERS(kstat.h)
dnl SystemV way to get uptime, highly portable
AC_CHECK_HEADERS(utmp.h)
dnl POSIX way to get uptime
AC_CHECK_HEADERS(utmpx.h)

dnl ######################################################################
dnl Fancy new Linux syscalls for file/data copying
dnl ######################################################################
AC_CHECK_HEADERS([linux/fs.h])
AC_CHECK_DECLS([FICLONE], [], [], [#include <linux/fs.h>])
AC_CHECK_DECLS([SEEK_DATA], [], [], [
#define _GNU_SOURCE
#include <unistd.h>
])
AC_CHECK_DECLS([FALLOC_FL_PUNCH_HOLE], [], [], [
#define _GNU_SOURCE
#include <fcntl.h>
])
AC_CHECK_HEADERS([sys/sendfile.h])
AC_CHECK_FUNCS([sendfile])
AC_CHECK_FUNCS([copy_file_range])


dnl #######################################################################
dnl Newer BSD systems don't have a compatible rtentry - use ortentry
dnl #######################################################################

rtry=none
AC_MSG_CHECKING(for either struct rtentry or struct ortentry)
AC_EGREP_HEADER(rtentry, net/route.h, rtry=rtentry)
if test "$rtry" = rtentry; then
 AC_DEFINE(HAVE_RTENTRY, 1, [Do we have any route entry structure?])
fi
AC_EGREP_HEADER(ortentry, net/route.h, rtry=ortentry)
if test "$rtry" = ortentry; then
 AC_DEFINE(HAVE_ORTENTRY, 1, [The old route entry structure in newer BSDs])
fi
AC_MSG_RESULT([$rtry])

dnl #######################################################################
dnl Enable extended attributes. Used for SELinux and ACLs
dnl #######################################################################

AC_CHECK_FUNCS(llistxattr, [AC_DEFINE(WITH_XATTR, 1, [Define if you have a libc that supports extended attributes])])
AC_CHECK_HEADERS([attr/xattr.h sys/xattr.h])

AC_MSG_CHECKING([whether xattr functions have extra arguments])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/types.h>
                                    #include <sys/xattr.h>],
                                    [(void)llistxattr("", 0, 0, 0);
                                    (void)lgetxattr("", "", 0, 0, 0, 0);
                                    (void)lsetxattr("", "", "", 0, 0, 0);
                                    (void)lremovexattr("", "", 0);])],
                  [AC_DEFINE(WITH_XATTR_EXTRA_ARGS, 1, [Define if your xattr implementation has extra arguments])]
                  [AC_MSG_RESULT([yes])],
                  [AC_MSG_RESULT([no])])

dnl #######################################################################
dnl Check for "fancy"(new) C compiler options
dnl #######################################################################

saved_CFLAGS="$CFLAGS"
CFLAGS="-Werror -Wno-tautological-constant-out-of-range-compare"
AC_MSG_CHECKING([whether compiler supports -Wno-tautological-constant-out-of-range-compare])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [NO_TAUTOLOGICAL_CC_OPTION="-Wno-tautological-constant-out-of-range-compare"],
    [AC_MSG_RESULT([no])]
    [NO_TAUTOLOGICAL_CC_OPTION=""]
)
CFLAGS="$saved_CFLAGS"
AC_SUBST([NO_TAUTOLOGICAL_CC_OPTION])

dnl ######################################################################
dnl Give the chance to enable SELINUX
dnl ######################################################################

AC_ARG_ENABLE([selinux], [AS_HELP_STRING([--enable-selinux], [Deprecated. SELinux support is always enabled])])

dnl ######################################################################
dnl OS specific stuff
dnl ######################################################################

case "$target_os" in

   solaris2.8|solaris2.9)
        AC_DEFINE(_XOPEN_SOURCE, 500, [UNIX 98])
        AC_DEFINE(__EXTENSIONS__, 1, [Extended UNIX 98 interfaces])
        ;;
   solaris2.10|solaris2.11)
        AC_DEFINE(_XOPEN_SOURCE, 600, [SUSv3])
        AC_DEFINE(__EXTENSIONS__, 1, [Extended UNIX 98 interfaces])
        ;;
   hpux*|hp-ux*)
        dnl pstat* functions may fail with EOVERFLOW without this.
        AC_DEFINE(_PSTAT64, 1, [Enable wide data structures everywhere])
        ;;
   aix*)
        CPPFLAGS="$CPPFLAGS -w"
        ;;
   linux*|*bsd*|*gnu*)
        AC_CHECK_LIB(nss_nis, yp_get_default_domain)
        ;;
   freebsd*|dragonfly*)
        ;;
   netbsd*)
        ;;
   unicos*)
        ;;
   cray*)
        ;;
   qnx*)
        ;;
   openbsd*|obsd*)
        ;;
   sysv4.2MP|unix_sv*)
        ;;
   cygwin*)
        ;;
   mingw*)
        ;;
   sco*)
        ;;
   darwin*)
        ;;
   *)
        AC_MSG_ERROR(Unknown system type $target_os)
        ;;
esac

m4_include([m4/cf3_platforms.m4])

dnl #####################################################################
dnl Configure directories
dnl #####################################################################

AC_ARG_WITH(workdir,
    [  --with-workdir=WORKDIR  default for internal (trusted) working directory ],
    [
        if test "x$withval" != x ; then
            WORKDIR="$withval"
            LOGDIR="$withval"
            PIDDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in WORKDIR

adl_RECURSIVE_EVAL("${WORKDIR}", WORKDIR)

AC_DEFINE_UNQUOTED(WORKDIR, "${WORKDIR}", [Workdir location])
AC_SUBST(workdir, "${WORKDIR}")

AC_ARG_WITH(masterdir,
    [  --with-masterdir=MASTERDIR  default for internal masterfiles directory ],
    [
        if test "x$withval" != x ; then
            MASTERDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in MASTERDIR

adl_RECURSIVE_EVAL("${MASTERDIR}", MASTERDIR)

AC_DEFINE_UNQUOTED(MASTERDIR, "${MASTERDIR}", [Masterfiles directory location])
AC_SUBST(masterdir, "${MASTERDIR}")

AC_ARG_WITH(inputdir,
    [  --with-inputdir=INPUTDIR  default for internal inputs directory ],
    [
        if test "x$withval" != x ; then
            INPUTDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in INPUTDIR

adl_RECURSIVE_EVAL("${INPUTDIR}", INPUTDIR)

AC_DEFINE_UNQUOTED(INPUTDIR, "${INPUTDIR}", [Inputs directory location])
AC_SUBST(inputdir, "${INPUTDIR}")

AC_ARG_WITH(datadir,
    [  --with-datadir=DATADIR  default for internal data directory ],
    [
        if test "x$withval" != x ; then
            DATADIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in DATADIR

adl_RECURSIVE_EVAL("${DATADIR}", DATADIR)

dnl There's a conflict on mingw where they have a type called DATADIR!
AC_DEFINE_UNQUOTED(CF_DATADIR, "${DATADIR}", [Datadir location])
AC_SUBST(datadir, "${DATADIR}")

AC_ARG_WITH(logdir,
    [  --with-logdir=LOGDIR  default for internal log directory ],
    [
        if test "x$withval" != x ; then
            LOGDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in LOGDIR

adl_RECURSIVE_EVAL("${LOGDIR}", LOGDIR)

AC_DEFINE_UNQUOTED(LOGDIR, "${LOGDIR}", [Logdir location])
AC_SUBST(logdir, "${LOGDIR}")

AC_ARG_WITH(piddir,
    [  --with-piddir=PIDDIR  default for internal pid directory ],
    [
        if test "x$withval" != x ; then
            PIDDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in PIDDIR

adl_RECURSIVE_EVAL("${PIDDIR}", PIDDIR)

AC_DEFINE_UNQUOTED(PIDDIR, "${PIDDIR}", [piddir location])
AC_SUBST(piddir, "${PIDDIR}")

AC_ARG_WITH(statedir,
    [  --with-statedir=STATEDIR  default for internal state directory ],
    [
        if test "x$withval" != x ; then
            STATEDIR="$withval"
        fi
    ],
)

dnl Expand ${prefix} and whatnot in STATEDIR

adl_RECURSIVE_EVAL("${STATEDIR}", STATEDIR)

AC_DEFINE_UNQUOTED(STATEDIR, "${STATEDIR}", [State directory location])
AC_SUBST(statedir, "${STATEDIR}")

AC_ARG_WITH(shell, [AS_HELP_STRING([--with-shell=PATH],
                   [Specify path to POSIX-compatible shell (if not /bin/sh)])],
                   [],
                   [with_shell=/bin/sh])

dnl Expand ${prefix} and whatnot in bindir
adl_RECURSIVE_EVAL("${bindir}", bindir)

AC_DEFINE_UNQUOTED(BINDIR, "${bindir}", [binaries location])
AC_SUBST(bindir, "${bindir}")


if test "x$with_shell" = "xno"; then
   AC_MSG_ERROR([Please specify full path to POSIX-compatible shell])
fi
AC_DEFINE_UNQUOTED(SHELL_PATH, "$with_shell", [Path to the POSIX-compatible shell])


dnl #####################################################################
dnl Hostname and Version stuff
dnl #####################################################################

AC_PATH_PROG(HOSTNAME, hostname, "", $PATH)

AC_DEFINE_UNQUOTED(AUTOCONF_HOSTNAME, "`$HOSTNAME`", [Special CFEngine symbol])
AC_DEFINE_UNQUOTED(AUTOCONF_SYSNAME, "$target_os", [Speial CFEngine symbol])


dnl #####################################################################
dnl xen cpuid-based hvm detection
dnl #####################################################################

AC_MSG_CHECKING(for Xen cpuid-based HVM detection)
if test x"$GCC" = "xyes"; then
    case $host_cpu in
        i[[3456]]86*|x86_64*|amd64)
            AC_DEFINE(XEN_CPUID_SUPPORT, 1, [Define if XEN cpuid-based HVM detection is available.])
            AC_MSG_RESULT(yes)
            ;;
        *)
            AC_MSG_RESULT(no)
            ;;
    esac
else
    AC_MSG_RESULT(no)
fi

dnl
dnl Code coverage
dnl

AC_ARG_ENABLE(coverage, AS_HELP_STRING([--enable-coverage], [Enable code coverage]),
              [use_coverage=$enableval], [use_coverage=no])

if test "x$use_coverage" = "xyes"; then
  if test "$GCC" != "yes"; then
    AC_MSG_ERROR([GCC is required for --enable-coverage])
  fi

  AC_CHECK_PROG(LCOV, lcov, lcov)
  AC_CHECK_PROG(LCOV_GENHTML, genhtml, genhtml)

  if test -z "$LCOV"; then
    AC_MSG_ERROR([Cannot find lcov from the LTP package])
  fi

  if test -z "$LCOV_GENHTML"; then
    AC_MSG_ERROR([Could not find genhtml from the LTP package])
  fi

  dnl Remove all optimization flags from CFLAGS
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9]*//g'`
  changequote([,])

  dnl Add the special gcc flags
  CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  LDFLAGS="$LDFLAGS -lgcov"
  # Need to set ENABLE_COVERAGE so that tests/unit/Makefile.am can adapt for one
  # test which needs gcov stubs if core not built with coverage.
  AM_CONDITIONAL([ENABLE_COVERAGE], true)
else
  AM_CONDITIONAL([ENABLE_COVERAGE], false)
fi

dnl ######################################################################
dnl Collect all the options
dnl ######################################################################

CORE_CPPFLAGS="$PCRE2_CPPFLAGS $OPENSSL_CPPFLAGS $LIBYAML_CPPFLAGS $CPPFLAGS"
CORE_CFLAGS="$PCRE2_CFLAGS $OPENSSL_CFLAGS $LIBYAML_CFLAGS $CFLAGS"
CORE_LDFLAGS="$PCRE2_LDFLAGS $OPENSSL_LDFLAGS $LIBYAML_LDFLAGS $LDFLAGS"
CORE_LIBS="$PCRE2_LIBS $OPENSSL_LIBS $LIBYAML_LIBS $LIBS"

dnl ######################################################################
dnl Make them available to subprojects.
dnl ######################################################################

AC_SUBST([CORE_CPPFLAGS])
AC_SUBST([CORE_CFLAGS])
AC_SUBST([CORE_LDFLAGS])
AC_SUBST([CORE_LIBS])

#
# Populate contents of config.post.h
#
AC_SUBST(post_macros)
AM_SUBST_NOTMAKE(post_macros)

dnl ######################################################################
dnl Check how file descriptor transfers are supported between proceses.
dnl ######################################################################
AC_CHECK_MEMBER([struct msghdr.msg_control],
                [AC_DEFINE([HAVE_MSGHDR_MSG_CONTROL], [1], [Define to 1 if SCM_RIGHTS supported])],
                [AC_DEFINE([HAVE_NO_MSGHDR_MSG_CONTROL], [1], [Define to 1 if SCM_RIGHTS support])],
                [[#include <sys/types.h>
                  #include <sys/socket.h>]])

AC_CHECK_MEMBER([struct msghdr.msg_accrights],
                [AC_DEFINE([HAVE_MSGHDR_ACCRIGHTS], [1], [Define to 1 if BSD .msg_accrights supported])],
                [AC_DEFINE([HAVE_NO_MSGHDR_ACCRIGHTS], [1], [Define to 1 if no BSD .msg_accrights support])],
                [[#include <sys/types.h>
                  #include <sys/socket.h>]])

dnl ######################################################################
dnl Summarize
dnl ######################################################################

AC_MSG_RESULT( )

AC_MSG_RESULT(Summary:)
AC_MSG_RESULT(> Version: AC_PACKAGE_VERSION)

AC_MSG_RESULT([> Optional libraries])

AC_MSG_RESULT([-> OpenSSL: $OPENSSL_PATH])

AC_MSG_RESULT([-> PCRE2: $PCRE2_PATH])

if test "x$ac_cv_lib_yaml_yaml_parser_initialize" = xyes; then
  AC_MSG_RESULT([-> libyaml: $LIBYAML_PATH])
else
  AC_MSG_RESULT([-> libyaml: disabled])
fi

AC_MSG_RESULT( )

dnl ######################################################################
dnl Now make the Makefiles
dnl ######################################################################

AC_CONFIG_FILES([Makefile
    libcompat/Makefile
    libutils/Makefile
    libutils/writer.h
    libutils/clockid_t.h
    libutils/regex.h
    libutils/glob_lib.h
    libutils/json-yaml.h
    config.post.h
    tests/Makefile
    tests/unit/Makefile])

AC_OUTPUT

AC_MSG_RESULT(DONE: Configuration done. Run make/gmake to build libntech.)
