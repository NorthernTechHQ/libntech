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
AM_CONDITIONAL([LINUX], [echo ${target_os} | grep -q linux])
AM_CONDITIONAL([MACOSX], [echo ${target_os} | grep -q darwin])
AM_CONDITIONAL([SOLARIS], [echo ${target_os} | $EGREP -q 'solaris|sunos'])
AM_CONDITIONAL([NT], [echo ${target_os} | $EGREP -q 'mingw|cygwin'])
AM_CONDITIONAL([CYGWIN], [echo ${target_os} | grep -q 'cygwin'])
AM_CONDITIONAL([AIX], [echo ${target_os} | grep -q aix])
AM_CONDITIONAL([HPUX], [echo ${target_os} | $EGREP -q 'hpux|hp-ux'])
AM_CONDITIONAL([FREEBSD], [echo ${target_os} | grep -q freebsd])
AM_CONDITIONAL([NETBSD], [echo ${target_os} | grep -q netbsd])
AM_CONDITIONAL([XNU], [echo ${target_os} | grep -q darwin])
