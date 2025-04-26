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
noinst_LTLIBRARIES = libutils.la

# TODO remove the openssl dependency! It's only there for base64 encoding.

libutils_la_LIBADD = ../libcompat/libcompat.la $(PCRE2_LIBS) $(OPENSSL_LIBS) $(SYSTEMD_LOGGING_LIBS) $(LIBYAML_LIBS)

libutils_la_SOURCES = \
	alloc.c alloc.h \
	array_map.c array_map_priv.h \
	buffer.c buffer.h \
	cleanup.c cleanup.h \
	clockid_t.h \
	compiler.h \
	csv_writer.c csv_writer.h \
	csv_parser.c csv_parser.h \
	definitions.h \
	deprecated.h \
	dir.h dir_priv.h \
	file_lib.c file_lib.h \
	fsattrs.c fsattrs.h \
	hash_map.c hash_map_priv.h \
	hash_method.h \
	ip_address.c ip_address.h \
	json.c json.h json-priv.h \
	json-pcre.h \
	json-utils.c json-utils.h \
	json-yaml.c json-yaml.h \
	known_dirs.c known_dirs.h \
	list.c list.h \
	logging.c logging.h logging_priv.h \
	man.c man.h \
	map.c map.h map_common.h \
	misc_lib.c misc_lib.h \
	mustache.c mustache.h \
	mutex.c mutex.h \
	passopenfile.c passopenfile.h \
	path.c path.h \
	platform.h condition_macros.h \
	printsize.h \
	proc_keyvalue.c proc_keyvalue.h \
	queue.c queue.h \
	rb-tree.c rb-tree.h \
	refcount.c refcount.h \
	ring_buffer.c ring_buffer.h \
	sequence.c sequence.h \
	string_sequence.c string_sequence.h \
	set.c set.h \
	stack.c stack.h \
	threaded_stack.c threaded_stack.h \
	statistics.c statistics.h \
	string_lib.c string_lib.h \
	threaded_deque.c threaded_deque.h \
	threaded_queue.c threaded_queue.h \
	unicode.c unicode.h \
	version_comparison.c version_comparison.h \
	writer.c writer.h \
	xml_writer.c xml_writer.h \
	glob_lib.c glob_lib.h

EXTRA_DIST = stack_base.c

if WITH_OPENSSL
libutils_la_SOURCES += \
	encode.c encode.h \
	hash.c hash.h
endif

if WITH_PCRE2
libutils_la_SOURCES += \
	regex.c regex.h
endif

if !NT
libutils_la_SOURCES += unix_dir.c
endif
if CYGWIN
libutils_la_SOURCES += unix_dir.c
endif

CLEANFILES = *.gcno *.gcda

#
# Some basic clean ups
#
MOSTLYCLEANFILES = *~ *.orig *.rej

#
# Get everything removed down to where rebuilding requires:
# "aclocal; autoconf; autoheader; automake --add-missing"
# "configure; make; make install"
#
MAINTAINERCLEANFILES = config.h.in
