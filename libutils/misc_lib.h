/*
  Copyright 2024 Northern.tech AS

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

#ifndef CFENGINE_MISC_LIB_H
#define CFENGINE_MISC_LIB_H

#include <compiler.h>
#include <stddef.h>						/* size_t */
#include <clockid_t.h>

#define ProgrammingError(...) __ProgrammingError(__FILE__, __LINE__, __VA_ARGS__)

/* TODO UnexpectedError() needs
 * to be rate limited to avoid spamming the console.  */
#ifndef NDEBUG
# define UnexpectedError(...) __ProgrammingError(__FILE__, __LINE__, __VA_ARGS__)
#else
# define UnexpectedError(...) __UnexpectedError(__FILE__, __LINE__, __VA_ARGS__)
#endif


/**
 *  CF_ASSERT(condition, message...)
 *
 *  If NDEBUG is defined then the #message is printed and execution continues,
 *  else execution aborts.
 */

# define CF_ASSERT(condition, ...)                                      \
    do {                                                                \
        if (!(condition))                                               \
            UnexpectedError(__VA_ARGS__);                               \
    } while(0)

/**
 * CF_ASSERT_FIX(condition, fix, message...)
 *
 * If NDEBUG is defined then the #message is printed, the #fix is executed,
 * and execution continues. If not NDEBUG, the #fix is ignored and execution
 * is aborted.
 */
# define CF_ASSERT_FIX(condition, fix, ...)                             \
    {                                                                   \
        if (!(condition))                                               \
        {                                                               \
            UnexpectedError(__VA_ARGS__);                               \
            (fix);                                                      \
        }                                                               \
    }


#define ISPOW2(n)     (  (n)>0  &&  ((((n) & ((n)-1)) == 0))  )

#define ABS(n)        (  (n<0)  ?  (-n)  :  (n)  )

/**
 * Hack to get optional- and required-arguments with short options to work the
 * same way.
 *
 *     If the option has an optional argument, it must be written
 *     directly after the option character if present.
 *      - getopt(1) - Linux man page
 *
 * This means that getopt expects an option "-o" with an optional argument
 * "foo" to be passed like this: "-ofoo". While an option "-r" with a
 * required argument "bar" is expected to be passed like this: "-r bar".
 *
 * Confusing, right? This little hack makes sure that the option-character and
 * the optional argument can also be separated by a whitespace-character. This
 * way optional- and required-arguments can be passed in the same fashion.
 *
 * This is how it works:
 * If optarg is NULL, we check whether the next string in argv is an option
 * (the variable optind has the index of the next element to be processed in
 * argv). If this is not the case, we assume it is an argument. The argument
 * is extracted and optind is advanced accordingly.
 *
 * This is how you use it:
 * ```
 * case 'o':
 *     if (OPTIONAL_ARGUMENT_IS_PRESENT)
 *     {
 *         // Handle is present
 *     }
 *     else
 *     {
 *         // Handle is not present
 *     }
 *     break;
 * ```
 */
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))

/*
  In contrast to the standard C modulus operator (%), this gives
  you an unsigned modulus. So where -1 % 3 => -1,
  UnsignedModulus(-1, 3) => 2.
*/
unsigned long UnsignedModulus(long dividend, long divisor);

size_t UpperPowerOfTwo(size_t v);


void __ProgrammingError(const char *file, int lineno, const char *format, ...) \
    FUNC_ATTR_PRINTF(3, 4) FUNC_ATTR_NORETURN;

void __UnexpectedError(const char *file, int lineno, const char *format, ...) \
    FUNC_ATTR_PRINTF(3, 4);


/**
 * Unchecked versions of common functions, i.e. functions that no longer
 * return anything, but try to continue in case of failure.
 *
 * @NOTE call these only with arguments that will always succeed!
 */


void xclock_gettime(clockid_t clk_id, struct timespec *ts);
void xsnprintf(char *str, size_t str_size, const char *format, ...);

int setenv_wrapper(const char *name, const char *value, int overwrite);
int putenv_wrapper(const char *str);

#endif
