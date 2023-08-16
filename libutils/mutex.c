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

#include <mutex.h>

#include <logging.h>                                            /* Log */
#include <cleanup.h>


void __ThreadLock(pthread_mutex_t *mutex,
                  const char *funcname, const char *filename, int lineno)

{
    int result = pthread_mutex_lock(mutex);

    if (result != 0)
    {
        /* Since Log blocks on mutexes, using it would be unsafe. Therefore,
           we use fprintf instead */
        fprintf(stderr,
                "Locking failure at %s:%d function %s! "
                "(pthread_mutex_lock: %s)",
                filename, lineno, funcname, GetErrorStrFromCode(result));
        fflush(stdout);
        fflush(stderr);
        DoCleanupAndExit(101);
    }
}

void __ThreadUnlock(pthread_mutex_t *mutex,
                    const char *funcname, const char *filename, int lineno)
{
    int result = pthread_mutex_unlock(mutex);

    if (result != 0)
    {
        /* Since Log blocks on mutexes, using it would be unsafe. Therefore,
           we use fprintf instead */
        fprintf(stderr,
                "Locking failure at %s:%d function %s! "
                "(pthread_mutex_unlock: %s)",
                filename, lineno, funcname, GetErrorStrFromCode(result));
        fflush(stdout);
        fflush(stderr);
        DoCleanupAndExit(101);
    }
}

int __ThreadWait(pthread_cond_t *pcond, pthread_mutex_t *mutex, int timeout,
                    const char *funcname, const char *filename, int lineno)
{
    int result = 0;

    if (timeout == THREAD_BLOCK_INDEFINITELY)
    {
        result = pthread_cond_wait(pcond, mutex);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_sec += timeout;
        result = pthread_cond_timedwait(pcond, mutex, &ts);
    }

    if (result != 0)
    {
        if (result == ETIMEDOUT)
        {
            Log(LOG_LEVEL_DEBUG,
                "Thread condition timed out at %s:%d function %s! "
                "(pthread_cond_timewait): %s)",
                filename, lineno, funcname, GetErrorStrFromCode(result));
        }
        else
        {
            /* Since Log blocks on mutexes, using it would be unsafe.
               Therefore, we use fprintf instead */
            fprintf(stderr,
                    "Failed to wait for thread condition at %s:%d function "
                    "%s! (pthread_cond_(wait|timewait)): %s)",
                    filename, lineno, funcname, GetErrorStrFromCode(result));
            fflush(stdout);
            fflush(stderr);
            DoCleanupAndExit(101);
        }
    }

    return result;
}
