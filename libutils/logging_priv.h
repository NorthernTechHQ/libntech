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

#ifndef CFENGINE_LOGGING_PRIV_H
#define CFENGINE_LOGGING_PRIV_H

/*
 * This interface is private and intended only for use by logging extensions (such as one defined in libpromises).
 */

typedef struct LoggingPrivContext LoggingPrivContext;

typedef char *(*LoggingPrivLogHook)(LoggingPrivContext *context, LogLevel level, const char *message);

struct LoggingPrivContext
{
    LoggingPrivLogHook log_hook;
    void *param;

    /**
     * Generally the log_hook runs whenever the message is printed either to
     * console or to syslog. You can set this to *additionally* run the hook
     * when the message level is <= force_hook_level.
     *
     * @NOTE the default setting of 0 equals to CRIT level, which is good as
     *       default since the CRIT messages are always printed anyway, so
     *       the log_hook runs anyway.
     */
    LogLevel force_hook_level;
};

/**
 * @brief Attaches context to logging for current thread
 */
void LoggingPrivSetContext(LoggingPrivContext *context);

/**
 * @brief Retrieves logging context for current thread
 */
LoggingPrivContext *LoggingPrivGetContext(void);

/**
 * @brief Set logging (syslog) and reporting (stdout) level for current thread
 */
void LoggingPrivSetLevels(LogLevel log_level, LogLevel report_level);

#endif
