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

#ifndef CFENGINE_ALLOC_H
#define CFENGINE_ALLOC_H

#include <stdlib.h> // size_t
#include <stdarg.h> // va_list
#include <assert.h> // assert()

#include <compiler.h>

void *xcalloc(size_t nmemb, size_t size);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str);
char *xstrndup(const char *str, size_t n);
void *xmemdup(const void *mem, size_t size);
int xasprintf(char **strp, const char *fmt, ...) FUNC_ATTR_PRINTF(2, 3);
int xvasprintf(char **strp, const char *fmt, va_list ap) FUNC_ATTR_PRINTF(2, 0);

static inline void free_array_items(void **array, size_t n_items)
{
    assert(array != NULL);
    for (size_t i = 0; i < n_items; i++)
    {
        free(array[i]);
    }
}

#define DESTROY_AND_NULL(destroy, ptr) { destroy(ptr); ptr = NULL; }
#define FREE_AND_NULL(ptr) { DESTROY_AND_NULL(free, ptr); }

/*
 * Prevent any code from using un-wrapped allocators.
 *
 * Use x* equivalents instead.
 */

/**
 * Currently regular malloc() calls are allowed for mission-critical code that
 * can somehow recover, like cf-serverd dropping connections or cf-execd
 * postponing its scheduled actions.
 *
 * @note for 99% of the cases (libpromises, cf-agent etc) use xmalloc() and
 *       friends.
 **/
#if 0

# undef malloc
# undef calloc
# undef realloc
# undef strdup
# undef strndup
# undef memdup
# undef asprintf
# undef vasprintf
# define malloc __error_unchecked_malloc
# define calloc __error_unchecked_calloc
# define realloc __error_unchecked_realloc
# define strdup __error_unchecked_strdup
# define strndup __error_unchecked_strndup
# define memdup __error_unchecked_memdup
# define asprintf __error_unchecked_asprintf
# define vasprintf __error_unchecked_vasprintf

void __error_unchecked_malloc(void);
void __error_unchecked_calloc(void);
void __error_unchecked_realloc(void);
void __error_unchecked_strdup(void);
void __error_unchecked_strndup(void);
void __error_unchecked_memdup(void);
void __error_unchecked_asprintf(void);
void __error_unchecked_vasprintf(void);

#endif

#endif
