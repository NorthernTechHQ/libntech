#
#  Copyright 2021 Northern.tech AS
#
#  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; version 3.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
#
# To the extent this program is licensed as part of the Enterprise
# versions of CFEngine, the applicable Commercial Open Source License
# (COSL) may apply to this file if you as a licensee so wish it. See
# included file COSL.txt.
#
# Just recursively include in dist tarball all data we need for unit tests
EXTRA_DIST = data

AM_CPPFLAGS = $(CORE_CPPFLAGS) \
	$(ENTERPRISE_CFLAGS) \
	-I$(srcdir)/../../libutils \
	-DTESTDATADIR='"$(srcdir)/data"'

LDADD = ../../libutils/libutils.la libtest.la

# automake does not support "maude_LIBS" variables. We can only alter
# the generic LIBS one. In case the functions are mocked in the test
# implementation, then we are pretty sure that they will be overriden by
# our local implementation. So we include *everything*...
LIBS = $(CORE_LIBS)
AM_LDFLAGS = $(CORE_LDFLAGS)

AM_CFLAGS = $(CORE_CFLAGS) $(PTHREAD_CFLAGS)


check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = cmockery.c cmockery.h schema.h test.c test.h \
	../../libutils/alloc.c \
	../../libutils/known_dirs.c \
	../../libutils/map.c \
	../../libutils/array_map.c \
	../../libutils/hash_map.c

if WITH_OPENSSL
libtest_la_SOURCES += \
	../../libutils/hash.c
endif

libtest_la_LIBADD = ../../libcompat/libcompat.la

check_LTLIBRARIES += libstr.la
libstr_la_SOURCES = \
	../../libutils/buffer.c \
	../../libutils/pcre_wrap.c \
	../../libutils/logging.c \
	../../libutils/misc_lib.c \
	../../libutils/regex.c \
	../../libutils/sequence.c \
	../../libutils/string_lib.c \
	../../libutils/writer.c
libstr_la_LIBADD = libtest.la

if WITH_OPENSSL
libstr_la_SOURCES += \
	../../libutils/encode.c
endif


check_PROGRAMS = \
	cleanup_test \
	condition_macros_test \
	csv_writer_test \
	set_test \
	csv_parser_test \
	env_file_test \
	alloc_test \
	string_writer_test \
	file_writer_test \
	xml_writer_test \
	sequence_test \
	json_test \
	misc_lib_test \
	string_lib_test \
	thread_test \
	file_lib_test \
	file_lock_test \
	map_test \
	path_test \
	logging_timestamp_test \
	refcount_test \
	list_test \
	buffer_test \
	ipaddress_test \
	rb-tree-test \
	queue_test \
	stack_test \
	threaded_queue_test \
	threaded_deque_test \
	threaded_stack_test \
	version_comparison_test \
	ring_buffer_test \
	libcompat_test \
	definitions_test

if WITH_OPENSSL
check_PROGRAMS += \
	hash_test
endif

TESTS = $(check_PROGRAMS)

#
# OS X uses real system calls instead of our stubs unless this option is used
#
TESTS_ENVIRONMENT = DYLD_FORCE_FLAT_NAMESPACE=yes

cleanup_test_SOURCES = cleanup_test.c

csv_writer_test_SOURCES = csv_writer_test.c ../../libutils/csv_writer.c \
	../../libutils/cleanup.c
csv_writer_test_LDADD = libtest.la libstr.la

xml_writer_test_SOURCES = xml_writer_test.c \
	../../libutils/xml_writer.c \
	../../libutils/cleanup.c
xml_writer_test_LDADD = libtest.la libstr.la

list_test_SOURCES = list_test.c

refcount_test_SOURCES = refcount_test.c ../../libutils/refcount.c

buffer_test_SOURCES = buffer_test.c ../../libutils/buffer.c
# Workaround for object file basename conflicts, search the web for
# "automake created with both libtool and without"
buffer_test_CPPFLAGS = $(AM_CPPFLAGS)

csv_parser_test_SOURCES = csv_parser_test.c ../../libutils/csv_parser.c
csv_parser_test_LDADD = libtest.la ../../libutils/libutils.la

ipaddress_test_SOURCES = ipaddress_test.c

#file_writer_test_CPPFLAGS = -I$(top_srcdir)/libutils
file_writer_test_SOURCES = file_writer_test.c
# ENABLE_COVERAGE is set in top-level configure.ac
if !ENABLE_COVERAGE
file_writer_test_SOURCES += gcov-stub.c
endif

CLEANFILES = *.gcno *.gcda cfengine-enterprise.so

file_lib_test_SOURCES = file_lib_test.c \
	../../libutils/file_lib.c \
	../../libutils/logging.c \
	../../libutils/misc_lib.c \
	../../libutils/path.c \
	../../libutils/string_lib.c \
	../../libutils/sequence.c \
	../../libutils/set.c \
	../../libutils/buffer.c \
	../../libutils/json.c \
	../../libutils/json-yaml.c \
	../../libutils/unix_dir.c \
	../../libutils/cleanup.c \
	../../libutils/writer.c

if WITH_PCRE
file_lib_test_SOURCES += \
	../../libutils/pcre_wrap.c \
	../../libutils/regex.c
endif

file_lib_test_LDADD = libtest.la
file_lib_test_CPPFLAGS = $(AM_CPPFLAGS) -DTEST_SYMLINK_ATOMICITY

logging_timestamp_test_SOURCES = logging_timestamp_test.c \
	../../libutils/logging.h
logging_timestamp_test_LDADD = libtest.la ../../libutils/libutils.la

hash_test_SOURCES = hash_test.c
hash_test_LDADD = ../../libutils/libutils.la libtest.la

libcompat_test_CPPFLAGS = -I$(top_srcdir)/libcompat -I$(top_srcdir)/libutils
libcompat_test_SOURCES = libcompat_test.c

# Silence some warnings about generated comparisions always being false:
# (The test generates some comparisons which compare an int to something
# bigger than INT_MAX)
misc_lib_test_CFLAGS = $(AM_CFLAGS) $(NO_TAUTOLOGICAL_CC_OPTION)

queue_test_SOURCES = queue_test.c

stack_test_SOURCES = stack_test.c

threaded_queue_test_SOURCES = threaded_queue_test.c

threaded_deque_test_SOURCES = threaded_deque_test.c

threaded_stack_test_SOURCES = threaded_stack_test.c

file_lock_test_SOURCES = file_lock_test.c
file_lock_test_LDADD = ../../libutils/libutils.la
