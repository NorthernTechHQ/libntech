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

#include <alloc.h>
#include <cleanup.h>
#include <platform.h>

typedef struct CleanupList
{
    CleanupFn fn;
    struct CleanupList *next;
} CleanupList;

static pthread_mutex_t cleanup_functions_mutex = PTHREAD_MUTEX_INITIALIZER;
static CleanupList *cleanup_functions;

/* To be called externally only by Windows binaries */
void CallCleanupFunctions(void)
{
    pthread_mutex_lock(&cleanup_functions_mutex);

    CleanupList *p = cleanup_functions;
    while (p)
    {
        CleanupList *cur = p;
        (cur->fn)();
        p = cur->next;
        free(cur);
    }

    cleanup_functions = NULL;

    pthread_mutex_unlock(&cleanup_functions_mutex);
}

void DoCleanupAndExit(int ret)
{
    CallCleanupFunctions();
    exit(ret);
}

void RegisterCleanupFunction(CleanupFn fn)
{
    pthread_mutex_lock(&cleanup_functions_mutex);

    CleanupList *p = xmalloc(sizeof(CleanupList));
    p->fn = fn;
    p->next = cleanup_functions;

    cleanup_functions = p;

    pthread_mutex_unlock(&cleanup_functions_mutex);
}

