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
# Just recursively include in dist tarball all data we need for unit tests
EXTRA_DIST = data

AM_CPPFLAGS = $(CORE_CPPFLAGS) \
	$(ENTERPRISE_CFLAGS) \
	-I$(srcdir)/../../libutils \
	-DTESTDATADIR='"$(srcdir)/data"'

LDADD = libtest.la

# automake does not support "maude_LIBS" variables. We can only alter
# the generic LIBS one. In case the functions are mocked in the test
# implementation, then we are pretty sure that they will be overriden by
# our local implementation. So we include *everything*...
LIBS = $(CORE_LIBS)
AM_LDFLAGS = $(CORE_LDFLAGS)

AM_CFLAGS = $(CORE_CFLAGS) $(PTHREAD_CFLAGS)


check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = cmockery.c cmockery.h schema.h test.c test.h

libtest_la_LIBADD = ../../libutils/libutils.la ../../libcompat/libcompat.la $(PCRE_LIBS) $(OPENSSL_LIBS) $(SYSTEMD_LOGGING_LIBS) $(LIBYAML_LIBS)

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
	fsattrs_test \
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
	definitions_test \
	glob_lib_test \
	string_sequence_test

if WITH_PCRE2
check_PROGRAMS += \
	regex_test
endif

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

csv_writer_test_SOURCES = csv_writer_test.c
csv_writer_test_LDADD = libtest.la

xml_writer_test_SOURCES = xml_writer_test.c
xml_writer_test_LDADD = libtest.la

list_test_SOURCES = list_test.c

refcount_test_SOURCES = refcount_test.c

buffer_test_SOURCES = buffer_test.c
# Workaround for object file basename conflicts, search the web for
# "automake created with both libtool and without"
buffer_test_CPPFLAGS = $(AM_CPPFLAGS)

csv_parser_test_SOURCES = csv_parser_test.c
csv_parser_test_LDADD = libtest.la

#file_writer_test_CPPFLAGS = -I$(top_srcdir)/libutils
file_writer_test_SOURCES = file_writer_test.c
# ENABLE_COVERAGE is set in top-level configure.ac
if !ENABLE_COVERAGE
file_writer_test_SOURCES += gcov-stub.c
endif

CLEANFILES = *.gcno *.gcda cfengine-enterprise.so

clean-local:
	rm -rf test_glob_file_list_??????

file_lib_test_SOURCES = file_lib_test.c
file_lib_test_CPPFLAGS = $(AM_CPPFLAGS) -DTEST_SYMLINK_ATOMICITY

ipaddress_test_SOURCES = ipaddress_test.c
ipaddress_test_CPPFLAGS = $(AM_CPPFLAGS)

logging_timestamp_test_SOURCES = logging_timestamp_test.c \
	../../libutils/logging.h

hash_test_SOURCES = hash_test.c

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

glob_lib_test_SOURCES = glob_lib_test.c

string_sequence_test_SOURCES = string_sequence_test.c
