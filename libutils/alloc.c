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

#define ALLOC_IMPL
#include <platform.h>
#include <alloc.h>
#include <cleanup.h>

static void *CheckResult(void *ptr, const char *fn, bool check_result)
{
    if ((ptr == NULL) && (check_result))
    {
        fputs(fn, stderr);
        fputs("CRITICAL: Unable to allocate memory\n", stderr);
        DoCleanupAndExit(255);
    }
    return ptr;
}

void *xmalloc(size_t size)
{
    return CheckResult(malloc(size), "xmalloc", size != 0);
}

void *xcalloc(size_t nmemb, size_t size)
{
    return CheckResult(calloc(nmemb, size), "xcalloc", (nmemb != 0) && (size != 0));
}

void *xrealloc(void *ptr, size_t size)
{
    return CheckResult(realloc(ptr, size), "xrealloc", size != 0);
}

char *xstrdup(const char *str)
{
    return CheckResult(strdup(str), "xstrdup", true);
}

char *xstrndup(const char *str, size_t n)
{
    return CheckResult(strndup(str, n), "xstrndup", true);
}

void *xmemdup(const void *data, size_t size)
{
    return CheckResult(memdup(data, size), "xmemdup", size != 0);
}

int xasprintf(char **strp, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int res = xvasprintf(strp, fmt, ap);

    va_end(ap);
    return res;
}

int xvasprintf(char **strp, const char *fmt, va_list ap)
{
    int res = vasprintf(strp, fmt, ap);

    CheckResult(res == -1 ? NULL : *strp, "xvasprintf", true);
    return res;
}
