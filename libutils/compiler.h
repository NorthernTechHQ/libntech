/*
  Copyright 2023 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#ifndef CFENGINE_COMPILER_H
#define CFENGINE_COMPILER_H

/* Compiler-specific options/defines */


#if defined(__GNUC__) && (__GNUC__ >= 3)


#  define FUNC_ATTR_NORETURN  __attribute__((noreturn))
#  define FUNC_ATTR_PRINTF(string_index, first_to_check) \
     __attribute__((format(__printf__, string_index, first_to_check)))
#  define FUNC_UNUSED __attribute__((unused))
#  define ARG_UNUSED __attribute__((unused))
#  define FUNC_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#  if (__GNUC__ >= 4) && (__GNUC_MINOR__ >=5)
#    define FUNC_DEPRECATED(msg) __attribute__((deprecated(msg)))
#  else
#    define FUNC_DEPRECATED(msg) __attribute__((deprecated))
#  endif


#else /* not gcc >= 3.0 */


#  define FUNC_ATTR_NORETURN
#  define FUNC_ATTR_PRINTF(string_index, first_to_check)
#  define FUNC_UNUSED
#  define ARG_UNUSED
#  define FUNC_WARN_UNUSED_RESULT

#  define FUNC_DEPRECATED(msg)


#endif  /* gcc >= 3.0 */



/**
 *  If you have a variable or function parameter unused under specific
 *  conditions (like ifdefs), you can suppress the "unused variable" warning
 *  by just doing UNUSED(x).
 */
#define UNUSED(x) (void)(x)


/**
 * For variables only used in debug builds, in particular only in assert()
 * calls, use NDEBUG_UNUSED.
 */
#ifdef NDEBUG
#define NDEBUG_UNUSED __attribute__((unused))
#else
#define NDEBUG_UNUSED
#endif

/**
 * If you want a string literal version of a macro, useful in scanf formats:
 *
 * #define BUFSIZE 1024
 * TO_STRING(BUFSIZE) -> "1024"
 */
#define STRINGIFY__INTERNAL_MACRO(x) #x
#define TO_STRING(x) STRINGIFY__INTERNAL_MACRO(x)

#endif  /* CFENGINE_COMPILER_H */
