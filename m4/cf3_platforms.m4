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
#
# OS kernels conditionals. Don't use those unless it is really needed (if code
# depends on the *kernel* feature, and even then -- some kernel features are
# shared by different kernels).
#
# Good example: use LINUX to select code which uses inotify and netlink sockets.
# Bad example: use LINUX to select code which parses output of coreutils' ps(1).
#
AM_CONDITIONAL([LINUX], [test -n "`echo ${target_os} | grep linux`"])
AM_CONDITIONAL([MACOSX], [test -n "`echo ${target_os} | grep darwin`"])
AM_CONDITIONAL([SOLARIS], [test -n "`(echo ${target_os} | egrep 'solaris|sunos')`"])
AM_CONDITIONAL([NT], [test -n "`(echo ${target_os} | egrep 'mingw|cygwin')`"])
AM_CONDITIONAL([CYGWIN], [test -n "`(echo ${target_os} | egrep 'cygwin')`"])
AM_CONDITIONAL([AIX], [test -n "`(echo ${target_os} | grep aix)`"])
AM_CONDITIONAL([HPUX], [test -n "`(echo ${target_os} | egrep 'hpux|hp-ux')`"])
AM_CONDITIONAL([FREEBSD], [test -n "`(echo ${target_os} | grep freebsd)`"])
AM_CONDITIONAL([NETBSD], [test -n "`(echo ${target_os} | grep netbsd)`"])
AM_CONDITIONAL([XNU], [test -n "`(echo ${target_os} | grep darwin)`"])
