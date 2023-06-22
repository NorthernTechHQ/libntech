/*
  Copyright 2023 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/


#ifndef CFENGINE_LOGGING_H
#define CFENGINE_LOGGING_H


#include <platform.h>
#include <compiler.h>


// Does not include timezone, since it is hard to match on Windows.
#define LOGGING_TIMESTAMP_REGEX "^20[0-9][0-9]-[01][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]"


typedef enum
{
    LOG_LEVEL_NOTHING = -1,
    LOG_LEVEL_CRIT,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_DEBUG
} LogLevel;


/**
 *  Enum used as a parameter in LogDebug(), used to print even more detailed
 *  info than Log(LOG_LEVEL_DEBUG).
 */
enum LogModule
{
    LOG_MOD_NONE = 0,                         /* DEFAULT DEBUG LOGGING */
    LOG_MOD_EVALCTX,                          /* evaluator */
    LOG_MOD_EXPAND,                           /* variables expansion */
    LOG_MOD_ITERATIONS,                       /* evaluator iteration engine */
    LOG_MOD_PARSER,                           /* yacc */
    LOG_MOD_VARTABLE,                         /* variables tables */
    LOG_MOD_VARS,                             /* variables promise */
    LOG_MOD_LOCKS,                            /* locks.c */
    LOG_MOD_PS,                               /* ps parsing */
    LOG_MOD_MAX
};


#include <logging_priv.h>


typedef struct
{
    LogLevel log_level;
    LogLevel report_level;
    bool color;

    LoggingPrivContext *pctx;
} LoggingContext;

const char *LogLevelToString(LogLevel level);
LogLevel LogLevelFromString(const char *level);

/**
 * @brief Return the standard timestamp format used in logging.
 * @param dest Output buffer
 * @param n size of output buffer
 * @param timestamp Timespec to format
 * @return True if successful, otherwise "<unknown>" will be printed to buffer
 */
bool LoggingFormatTimestamp(char dest[64], size_t n, struct tm *timestamp);

LoggingContext *GetCurrentThreadContext(void);
void LoggingFreeCurrentThreadContext(void);

/**
 * Whether a message with level #level would be logged by Log() or not.
 */
bool WouldLog(LogLevel level);

void Log(LogLevel level, const char *fmt, ...) FUNC_ATTR_PRINTF(2, 3);
void LogDebug(enum LogModule mod, const char *fmt, ...) FUNC_ATTR_PRINTF(2, 3);
void LogRaw(LogLevel level, const char *prefix, const void *buf, size_t buflen);
void VLog(LogLevel level, const char *fmt, va_list ap);

void LoggingSetAgentType(const char *type);
void LoggingEnableTimestamps(bool enable);

/**
 * The functions below work with two internal variables -- global_level and
 * global_system_log_level. If the latter one is not set, global_level is used
 * for both system log logging and report (console) logging. If it is set, it is
 * used for system log logging in all new threads unless/until
 * LoggingPrivSetLevels() is called in those threads.
 */
void LogSetGlobalLevel(LogLevel level);
void LogSetGlobalLevelArgOrExit(const char *const arg);
LogLevel LogGetGlobalLevel(void);
void LogSetGlobalSystemLogLevel(LogLevel level);
LogLevel LogGetGlobalSystemLogLevel(void);
void LogUnsetGlobalSystemLogLevel(void);

void LoggingSetColor(bool enabled);

/*
 * Portable syslog()
 */
void LogToSystemLog(const char *msg, LogLevel level);

/**
 * @brief Log a message with structured data to system log.
 * @details This function takes a variable-length argument list and expects
 *          structured data passed as key-value pairs, where both keys and
 *          values are C-strings. Furthermore, the last pair must contain the
 *          "MESSAGE" key, and the corresponding value accepts a format-string
 *          followed by its arguments. Format-strings are not accepted in the
 *          value of any other pair. The log priority (i.e., the pair
 *          containing the "PRIORITY" key) is automatically deduced from the
 *          log level argument and added to the structured log message.
 * @note Only the formatted string from the pair with the "MESSAGE" key is
 *       logged on platforms that does not support structured logging.
 * @param[in] level Log level.
 * @param[in] vararg Variable-length argument containing key-value pairs.
 */
void LogToSystemLogStructured(LogLevel level, ...);

/*
 * Portable strerror(errno)
 */
const char *GetErrorStr(void);
const char *GetErrorStrFromCode(int error_code);

void LogModuleHelp(void);
bool LogModuleEnabled(enum LogModule mod);
void LogEnableModule(enum LogModule mod);
bool LogEnableModulesFromString(char *s);

// byte_magnitude and byte_unit are used to print readable byte counts
long byte_magnitude(long bytes);
const char *byte_unit(long bytes);

/**
 * API for logging messages into a buffer which is later either committed
 * (messages are actually logged) or discarded.
 *
 * @note StartLoggingIntoBuffer() needs to be called first and then every time
 *       after DiscardLogBuffer() or CommitLogBuffer().
 * @note There's only one global buffer, this API is *not* thread-safe.
 */
/**
 * Enable logging into a buffer for all messages with the log level greater or
 * equal to #min_level and less or equal than #max_level (keep in mind that the
 * log levels are sorted from %LOG_LEVEL_CRIT, smallest, to %LOG_LEVEL_DEBUG,
 * greatest).
 */
void StartLoggingIntoBuffer(LogLevel min_level, LogLevel max_level);
void DiscardLogBuffer();
void CommitLogBuffer();

#endif
