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

#include <misc_lib.h>

#include <platform.h>
#include <alloc.h>
#include <string_lib.h>
#include <logging.h>
#include <errno.h>
#include <cleanup.h>

#include <stdarg.h>

unsigned long UnsignedModulus(long dividend, long divisor)
{
    return ((dividend % divisor) + divisor) % divisor;
}

size_t UpperPowerOfTwo(size_t v)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

void __ProgrammingError(const char *file, int lineno, const char *format, ...)
{
    va_list ap;
    char *fmt = NULL;

    va_start(ap, format);
    xasprintf(&fmt, "%s:%d: Programming Error: %s\n", file, lineno, format);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    free(fmt);
#ifdef NDEBUG
    DoCleanupAndExit(255);
#else
    abort();
#endif
}

/**
  @brief Log unexpected runtime error to stderr, do not exit program.
*/

void __UnexpectedError(const char *file, int lineno, const char *format, ...)
{
    va_list ap;
    char *fmt = NULL;

    va_start(ap, format);
    xasprintf(&fmt,
              "%s:%d: Unexpected Error - this is a BUG, please report it: %s\n",
              file, lineno, format);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    free(fmt);
}



void xclock_gettime(clockid_t clk_id, struct timespec *ts)
{
    int ret = clock_gettime(clk_id, ts);
    if (ret != 0)
    {
        Log(LOG_LEVEL_VERBOSE,
            "clock_gettime() failed (%s), falling back to time()",
            GetErrorStr());
        *ts = (struct timespec) { .tv_sec = time(NULL) };
    }
}

/**
 * Unchecked version of snprintf(). For when you're *sure* the result fits in
 * the buffer, and you don't want to check it. In other words, NO PART OF THE
 * OUTPUT SHOULD BE DEPENDENT ON USER DATA!
 *
 * Only exception is usage in the unit tests, where we use it all over the
 * place in order to flag stupid programming mistakes.
 */
void xsnprintf(char *str, size_t str_size, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    int ret = vsnprintf(str, str_size, format, ap);
    va_end(ap);

    if (ret < 0)                                                /* error */
    {
        *str = '\0';
        Log(LOG_LEVEL_WARNING, "Unexpected failure from snprint(\"%s\"): %s",
            format, GetErrorStr());
    }
    else if ((size_t) ret >= str_size)                           /* output truncated */
    {
#ifdef NDEBUG
        UnexpectedError("Result of snprintf(\"%s\") truncated at %zu chars",
                        format, str_size);
#else
        ProgrammingError("Result of snprintf(\"%s\") truncated at %zu chars",
                         format, str_size);
#endif
    }
}

int setenv_wrapper(const char *name, const char *value, int overwrite)
{
#if defined(__linux__) || defined(__APPLE__)
    return setenv(name, value, overwrite);
#else
    if (NULL_OR_EMPTY(name) || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }
    if (overwrite == 0 && getenv(name) != NULL)
    {
        return 0; // Don't overwrite
    }

    const size_t buffer_size = 1024;
    const char buf[buffer_size];
    int string_length = snprintf(buf, buffer_size, "%s=%s", name, value);
    if (string_length >= buffer_size)
    {
        errno = EINVAL;
        return -1; // Combined string is too long
    }

    // Fixing this leak on platforms which don't have setenv is difficult(!)
    return putenv(xstrdup(buf));
#endif
}

int putenv_wrapper(const char* str)
{
    char *const name = xstrdup(str);
    char *const equal_sign = strchr(name, '=');
    if (equal_sign == NULL)
    {
        free(name);
        errno = EINVAL;
        return -1;
    }
    *equal_sign = '\0';
    char *value = equal_sign + 1;
    const int ret = setenv_wrapper(name, value, 1);
    free(name);
    return ret;
}
