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

#ifndef CFENGINE_MUTEX_H
#define CFENGINE_MUTEX_H

#include <pthread.h>

#define THREAD_BLOCK_INDEFINITELY  -1

#define ThreadLock(m)       __ThreadLock(m, __func__, __FILE__, __LINE__)
#define ThreadUnlock(m)   __ThreadUnlock(m, __func__, __FILE__, __LINE__)
#define ThreadWait(m, n, t) __ThreadWait(m, n, t, __func__, __FILE__, __LINE__)

void __ThreadLock(pthread_mutex_t *mutex,
                  const char *funcname, const char *filename, int lineno);
void __ThreadUnlock(pthread_mutex_t *mutex,
                    const char *funcname, const char *filename, int lineno);

/**
  @brief Function to wait for `timeout` seconds or until signalled.
  @note Can use THREAD_BLOCK_INDEFINITELY to block until a signal is received.
  @param [in] cond Thread condition to wait for.
  @param [in] mutex Mutex lock to acquire once condition is met.
  @param [in] timeout Seconds to wait for, can be THREAD_BLOCK_INDEFINITELY
  @return 0 on success, -1 if timed out, exits if locking fails
 */
int __ThreadWait(pthread_cond_t *cond, pthread_mutex_t *mutex, int timeout,
                 const char *funcname, const char *filename, int lineno);

#endif
