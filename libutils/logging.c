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

#include <logging.h>
#include <alloc.h>
#include <string_lib.h>
#include <misc_lib.h>
#include <cleanup.h>
#include <sequence.h>

#if defined(HAVE_SYSTEMD_SD_JOURNAL_H) && defined(HAVE_LIBSYSTEMD)
#include <systemd/sd-journal.h> /* sd_journal_sendv() */
#endif // defined(HAVE_SYSTEMD_SD_JOURNAL_H) && defined(HAVE_LIBSYSTEMD)

#include <definitions.h>        /* CF_BUFSIZE */

char VPREFIX[1024] = ""; /* GLOBAL_C */

static char AgentType[80] = "generic";
static bool TIMESTAMPS = false;

static LogLevel global_level = LOG_LEVEL_NOTICE; /* GLOBAL_X */
static LogLevel global_system_log_level = LOG_LEVEL_NOTHING; /* default value that means not set */

static pthread_once_t log_context_init_once = PTHREAD_ONCE_INIT; /* GLOBAL_T */
static pthread_key_t log_context_key; /* GLOBAL_T, initialized by pthread_key_create */

static Seq *log_buffer = NULL;
static bool logging_into_buffer = false;
static LogLevel min_log_into_buffer_level = LOG_LEVEL_NOTHING;
static LogLevel max_log_into_buffer_level = LOG_LEVEL_NOTHING;

typedef struct LogEntry_
{
    LogLevel level;
    char *msg;
} LogEntry;

static void LoggingInitializeOnce(void)
{
    if (pthread_key_create(&log_context_key, &free) != 0)
    {
        /* There is no way to signal error out of pthread_once callback.
         * However if pthread_key_create fails we are pretty much guaranteed
         * that nothing else will work. */

        fprintf(stderr, "Unable to initialize logging subsystem\n");
        DoCleanupAndExit(255);
    }
}

LoggingContext *GetCurrentThreadContext(void)
{
    pthread_once(&log_context_init_once, &LoggingInitializeOnce);
    LoggingContext *lctx = pthread_getspecific(log_context_key);
    if (lctx == NULL)
    {
        lctx = xcalloc(1, sizeof(LoggingContext));
        lctx->log_level = (global_system_log_level != LOG_LEVEL_NOTHING ?
                           global_system_log_level :
                           global_level);
        lctx->report_level = global_level;
        pthread_setspecific(log_context_key, lctx);
    }
    return lctx;
}

void LoggingFreeCurrentThreadContext(void)
{
    pthread_once(&log_context_init_once, &LoggingInitializeOnce);
    LoggingContext *lctx = pthread_getspecific(log_context_key);
    if (lctx == NULL)
    {
        return;
    }
    // lctx->pctx is usually stack allocated and shouldn't be freed
    FREE_AND_NULL(lctx);
    pthread_setspecific(log_context_key, NULL);
}

void LoggingSetAgentType(const char *type)
{
    strlcpy(AgentType, type, sizeof(AgentType));
}

void LoggingEnableTimestamps(bool enable)
{
    TIMESTAMPS = enable;
}

void LoggingPrivSetContext(LoggingPrivContext *pctx)
{
    LoggingContext *lctx = GetCurrentThreadContext();
    lctx->pctx = pctx;
}

LoggingPrivContext *LoggingPrivGetContext(void)
{
    LoggingContext *lctx = GetCurrentThreadContext();
    return lctx->pctx;
}

void LoggingPrivSetLevels(LogLevel log_level, LogLevel report_level)
{
    LoggingContext *lctx = GetCurrentThreadContext();
    lctx->log_level = log_level;
    lctx->report_level = report_level;
}

const char *LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_CRIT:
        return "CRITICAL";
    case LOG_LEVEL_ERR:
        return "error";
    case LOG_LEVEL_WARNING:
        return "warning";
    case LOG_LEVEL_NOTICE:
        return "notice";
    case LOG_LEVEL_INFO:
        return "info";
    case LOG_LEVEL_VERBOSE:
        return "verbose";
    case LOG_LEVEL_DEBUG:
        return "debug";
    default:
        ProgrammingError("LogLevelToString: Unexpected log level %d", level);
    }
}

LogLevel LogLevelFromString(const char *const level)
{
    // Only compare the part the user typed
    // i/info/inform/information will all result in LOG_LEVEL_INFO
    size_t len = SafeStringLength(level);
    if (len == 0)
    {
        return LOG_LEVEL_NOTHING;
    }
    if (StringEqualN_IgnoreCase(level, "CRITICAL", len))
    {
        return LOG_LEVEL_CRIT;
    }
    if (StringEqualN_IgnoreCase(level, "errors", len))
    {
        return LOG_LEVEL_ERR;
    }
    if (StringEqualN_IgnoreCase(level, "warnings", len))
    {
        return LOG_LEVEL_WARNING;
    }
    if (StringEqualN_IgnoreCase(level, "notices", len))
    {
        return LOG_LEVEL_NOTICE;
    }
    if (StringEqualN_IgnoreCase(level, "information", len))
    {
        return LOG_LEVEL_INFO;
    }
    if (StringEqualN_IgnoreCase(level, "verbose", len))
    {
        return LOG_LEVEL_VERBOSE;
    }
    if (StringEqualN_IgnoreCase(level, "debug", len))
    {
        return LOG_LEVEL_DEBUG;
    }
    return LOG_LEVEL_NOTHING;
}

static const char *LogLevelToColor(LogLevel level)
{

    switch (level)
    {
    case LOG_LEVEL_CRIT:
    case LOG_LEVEL_ERR:
        return "\x1b[31m"; // red

    case LOG_LEVEL_WARNING:
        return "\x1b[33m"; // yellow

    case LOG_LEVEL_NOTICE:
    case LOG_LEVEL_INFO:
        return "\x1b[32m"; // green

    case LOG_LEVEL_VERBOSE:
    case LOG_LEVEL_DEBUG:
        return "\x1b[34m"; // blue

    default:
        ProgrammingError("LogLevelToColor: Unexpected log level %d", level);
    }
}

bool LoggingFormatTimestamp(char dest[64], size_t n, struct tm *timestamp)
{
    if (strftime(dest, n, "%Y-%m-%dT%H:%M:%S%z", timestamp) == 0)
    {
        strlcpy(dest, "<unknown>", n);
        return false;
    }
    return true;
}

static void LogToConsole(const char *msg, LogLevel level, bool color)
{
    struct tm now;
    time_t now_seconds = time(NULL);
    localtime_r(&now_seconds, &now);

    if (color)
    {
        fprintf(stdout, "%s", LogLevelToColor(level));
    }
    if (level >= LOG_LEVEL_INFO && VPREFIX[0])
    {
        fprintf(stdout, "%s ", VPREFIX);
    }
    if (TIMESTAMPS)
    {
        char formatted_timestamp[64];
        LoggingFormatTimestamp(formatted_timestamp, 64, &now);
        fprintf(stdout, "%s ", formatted_timestamp);
    }

    fprintf(stdout, "%8s: %s\n", LogLevelToString(level), msg);

    if (color)
    {
        // Turn off the color again.
        fprintf(stdout, "\x1b[0m");
    }

    fflush(stdout);
}

#if !defined(__MINGW32__)
static int LogLevelToSyslogPriority(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_CRIT: return LOG_CRIT;
    case LOG_LEVEL_ERR: return LOG_ERR;
    case LOG_LEVEL_WARNING: return LOG_WARNING;
    case LOG_LEVEL_NOTICE: return LOG_NOTICE;
    case LOG_LEVEL_INFO: return LOG_INFO;
    case LOG_LEVEL_VERBOSE: return LOG_DEBUG; /* FIXME: Do we really want to conflate those levels? */
    case LOG_LEVEL_DEBUG: return LOG_DEBUG;
    default:
        ProgrammingError("LogLevelToSyslogPriority: Unexpected log level %d",
                         level);
    }

}

void LogToSystemLog(const char *msg, LogLevel level)
{
    char logmsg[4096];
    snprintf(logmsg, sizeof(logmsg), "CFEngine(%s) %s %s\n", AgentType, VPREFIX, msg);
    syslog(LogLevelToSyslogPriority(level), "%s", logmsg);
}
#endif  /* !__MINGW32__ */

#if defined(HAVE_SYSTEMD_SD_JOURNAL_H) && defined(HAVE_LIBSYSTEMD)
void LogToSystemLogStructured(const int level, ...)
{
    va_list args;
    va_start(args, level);

    // Additional pairs
    Seq *const seq = SeqNew(2, free);
    for (const char *key = va_arg(args, char *); !StringEqual(key, "MESSAGE");
         key = va_arg(args, char *))
    {
        const char *const value = va_arg(args, char *);
        char *const pair = StringFormat("%s=%s", key, value);
        SeqAppend(seq, pair);
    }

    // Message pair
    const char *const format_str = va_arg(args, char *);
    char *const message = StringVFormat(format_str, args);
    char *pair = StringFormat("MESSAGE=%s", message);
    free(message);
    SeqAppend(seq, pair);

    // Priority pair
    const int priority = LogLevelToSyslogPriority(level);
    pair = StringFormat("PRIORITY=%i", priority);
    SeqAppend(seq, pair);

    const int num_pairs = SeqLength(seq);
    struct iovec iov[num_pairs];
    for (int i = 0; i < num_pairs; i++)
    {
        iov[i].iov_base = SeqAt(seq, i);
        iov[i].iov_len = strlen(iov[i].iov_base);
    }

    NDEBUG_UNUSED int ret = sd_journal_sendv(iov, num_pairs);
    assert(ret == 0);
    SeqDestroy(seq);
    va_end(args);
}
#else // defined(HAVE_SYSTEMD_SD_JOURNAL_H) && defined(HAVE_LIBSYSTEMD)
void LogToSystemLogStructured(const int level, ...)
{
    va_list args;
    va_start(args, level);

    for (const char *key = va_arg(args, char *); !StringEqual(key, "MESSAGE");
         key = va_arg(args, char *))
    {
        va_arg(args, char *);
    }

    const char *const format_str = va_arg(args, char *);
    char *const message = StringVFormat(format_str, args);
    LogToSystemLog(message, level);
    free(message);
    va_end(args);
}
#endif // defined(HAVE_SYSTEMD_SD_JOURNAL_H) && defined(HAVE_LIBSYSTEMD)

void __ImproperUseOfLogToSystemLogStructured(void)
{
    // TODO: CFE-4190
    //  - Make sure CodeQL finds at least the reqired function calls below,
    //    then remove this code
    assert(false); // Do not use this function!

    // Required: Missing required "MESSAGE" key.
    LogToSystemLogStructured(LOG_LEVEL_DEBUG, "FOO", "bogus", "BAR", "doofus");

    // Required: String-literal "MESSAGE" is not the same as "message".
    LogToSystemLogStructured(LOG_LEVEL_INFO, "FOO", "bogus", "BAR", "doofus", "message", "Hello CFEngine!");

    // Required: Number of format-string arguments is less than number of format specifiers.
    LogToSystemLogStructured(LOG_LEVEL_ERR, "MESSAGE", "%s %d", "CFEngine");

    // Optional: Number of format-string arguments is greated than number of format specifiers.
    LogToSystemLogStructured(LOG_LEVEL_ERR, "MESSAGE", "%s %d", "CFEngine", 123, "ROCKS!");

    // Optional: All keys must be uppercase or else they are ignored.
    LogToSystemLogStructured(LOG_LEVEL_CRIT, "FOO", "bogus", "bar", "DOOFUS", "MESSAGE", "Hello CFEngine!");

    // Optional: Pairs trailing the "MESSAGE" key are ignored.
    LogToSystemLogStructured(LOG_LEVEL_NOTICE, "FOO", "bogus", "MESSAGE", "Hello CFEngine!", "BAR", "doofus");
}

#ifndef __MINGW32__
const char *GetErrorStrFromCode(int error_code)
{
    return strerror(error_code);
}

const char *GetErrorStr(void)
{
    return strerror(errno);
}

#else

const char *GetErrorStrFromCode(int error_code)
{
    static char errbuf[CF_BUFSIZE];
    int len;

    memset(errbuf, 0, sizeof(errbuf));

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code,
                  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), errbuf, CF_BUFSIZE, NULL))
    {
        // remove CRLF from end
        len = strlen(errbuf);
        errbuf[len - 2] = errbuf[len - 1] = '\0';
    } else {
        strcpy(errbuf, "Unknown error");
    }

    return errbuf;
}

const char *GetErrorStr(void)
{
    return GetErrorStrFromCode(GetLastError());
}
#endif  /* !__MINGW32__ */

bool WouldLog(LogLevel level)
{
    LoggingContext *lctx = GetCurrentThreadContext();

    bool log_to_console = (level <= lctx->report_level);
    bool log_to_syslog  = (level <= lctx->log_level &&
                           level < LOG_LEVEL_VERBOSE);
    bool force_hook     = (lctx->pctx &&
                           lctx->pctx->log_hook &&
                           lctx->pctx->force_hook_level >= level);

    return (log_to_console || log_to_syslog || force_hook);
}

/**
 * #no_format determines whether #fmt_msg is interpreted as a format string and
 * combined with #ap to create the real log message (%false) or as a log message
 * dirrectly (%true) in which case #ap is ignored.
 *
 * @see LogNoFormat()
 */
static void VLogNoFormat(LogLevel level, const char *fmt_msg, va_list ap, bool no_format)
{
    LoggingContext *lctx = GetCurrentThreadContext();

    bool log_to_console = ( level <= lctx->report_level );
    bool log_to_syslog  = ( level <= lctx->log_level &&
                            level < LOG_LEVEL_VERBOSE );
    bool force_hook     = ( lctx->pctx &&
                            lctx->pctx->log_hook &&
                            lctx->pctx->force_hook_level >= level );

    /* NEEDS TO BE IN SYNC WITH THE CONDITION IN WouldLog() ABOVE! */
    if (!log_to_console && !log_to_syslog && !force_hook)
    {
        return;                            /* early return - save resources */
    }

    char *msg;
    if (no_format)
    {
        msg = xstrdup(fmt_msg);
    }
    else
    {
        msg = StringVFormat(fmt_msg, ap);
    }
    char *hooked_msg = NULL;

    if (logging_into_buffer &&
        (level >= min_log_into_buffer_level) && (level <= max_log_into_buffer_level))
    {
        assert(log_buffer != NULL);

        if (log_buffer == NULL)
        {
            /* Should never happen. */
            Log(LOG_LEVEL_ERR,
                "Attempt to log a message to an unitialized log buffer, discarding the message");
        }

        LogEntry *entry = xmalloc(sizeof(LogEntry));
        entry->level = level;
        entry->msg = msg;

        SeqAppend(log_buffer, entry);
        return;
    }

    /* Remove ending EOLN. */
    for (char *sp = msg; *sp != '\0'; sp++)
    {
        if (*sp == '\n' && *(sp+1) == '\0')
        {
            *sp = '\0';
            break;
        }
    }

    if (lctx->pctx && lctx->pctx->log_hook)
    {
        hooked_msg = lctx->pctx->log_hook(lctx->pctx, level, msg);
    }
    else
    {
        hooked_msg = msg;
    }

    if (log_to_console)
    {
        LogToConsole(hooked_msg, level, lctx->color);
    }
    if (log_to_syslog)
    {
        LogToSystemLogStructured(level, "MESSAGE", "%s", hooked_msg);
    }

    if (hooked_msg != msg)
    {
        free(hooked_msg);
    }
    free(msg);
}

void VLog(LogLevel level, const char *fmt, va_list ap)
{
    VLogNoFormat(level, fmt, ap, false);
}

/**
 * @brief Logs binary data in #buf, with unprintable bytes translated to '.'.
 *        Message is prefixed with #prefix.
 * @param #buflen must be no more than CF_BUFSIZE
 */
void LogRaw(LogLevel level, const char *prefix, const void *buf, size_t buflen)
{
    if (buflen > CF_BUFSIZE)
    {
        debug_abort_if_reached();
        buflen = CF_BUFSIZE;
    }

    LoggingContext *lctx = GetCurrentThreadContext();
    if (level <= lctx->report_level || level <= lctx->log_level)
    {
        const unsigned char *src = buf;
        unsigned char dst[CF_BUFSIZE+1];
        assert(buflen < sizeof(dst));
        size_t i;
        for (i = 0; i < buflen; i++)
        {
            dst[i] = isprint(src[i]) ? src[i] : '.';
        }
        assert(i < sizeof(dst));
        dst[i] = '\0';

        /* And Log the translated buffer, which is now a valid string. */
        Log(level, "%s%s", prefix, dst);
    }
}

void Log(LogLevel level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VLog(level, fmt, ap);
    va_end(ap);
}

/**
 * The same as above, but for logging messages without formatting sequences
 * ("%s", "%d",...). Or more precisely, for *safe* logging of messages that may
 * contain such sequences (for example Log(LOG_LEVEL_ERR, "%s") can potentially
 * log some secrets).
 *
 * It doesn't have the FUNC_ATTR_PRINTF restriction that causes a compilation
 * warning/error if #msg is not a constant string.
 *
 * @note This is for special cases where #msg was already printf-like formatted,
 *       the variadic arguments are ignored, they are here just so that
 *       'va_list ap' can be constructed and passed to VLogNoFormat().
 *
 * @see CommitLogBuffer()
 */
static void LogNoFormat(LogLevel level, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    VLogNoFormat(level, msg, ap, true);
    va_end(ap);
}

static bool module_is_enabled[LOG_MOD_MAX];
static const char *log_modules[LOG_MOD_MAX] =
{
    "",
    "evalctx",
    "expand",
    "iterations",
    "parser",
    "vartable",
    "vars",
    "locks",
    "ps",
};

static enum LogModule LogModuleFromString(const char *s)
{
    for (enum LogModule i = 0; i < LOG_MOD_MAX; i++)
    {
        if (strcmp(log_modules[i], s) == 0)
        {
            return i;
        }
    }

    return LOG_MOD_NONE;
}

void LogEnableModule(enum LogModule mod)
{
    assert(mod < LOG_MOD_MAX);

    module_is_enabled[mod] = true;
}

void LogModuleHelp(void)
{
    printf("\n--log-modules accepts a comma separated list of one or more of the following:\n\n");
    printf("    help\n");
    printf("    all\n");
    for (enum LogModule i = LOG_MOD_NONE + 1;  i < LOG_MOD_MAX;  i++)
    {
        printf("    %s\n", log_modules[i]);
    }
    printf("\n");
}

/**
 * Parse a string of modules, and enable the relevant DEBUG logging modules.
 * Example strings:
 *
 *   all         : enables all debug modules
 *   help        : enables nothing, but prints a help message
 *   iterctx     : enables the "iterctx" debug logging module
 *   iterctx,vars: enables the 2 debug modules, "iterctx" and "vars"
 *
 * @NOTE modifies string #s but restores it before returning.
 */
bool LogEnableModulesFromString(char *s)
{
    bool retval = true;

    const char *token = s;
    char saved_sep = ',';                     /* any non-NULL value will do */
    while (saved_sep != '\0' && retval != false)
    {
        char *next_token = strchrnul(token, ',');
        saved_sep        = *next_token;
        *next_token      = '\0';                      /* modify parameter s */
        size_t token_len = next_token - token;

        if (strcmp(token, "help") == 0)
        {
            LogModuleHelp();
            retval = false;                                   /* early exit */
        }
        else if (strcmp(token, "all") == 0)
        {
            for (enum LogModule j = LOG_MOD_NONE + 1; j < LOG_MOD_MAX; j++)
            {
                LogEnableModule(j);
            }
        }
        else
        {
            enum LogModule mod = LogModuleFromString(token);

            assert(mod < LOG_MOD_MAX);
            if (mod == LOG_MOD_NONE)
            {
                Log(LOG_LEVEL_WARNING,
                    "Unknown debug logging module '%*s'",
                    (int) token_len, token);
            }
            else
            {
                LogEnableModule(mod);
            }
        }


        *next_token = saved_sep;            /* restore modified parameter s */
        next_token++;                       /* bypass comma */
        token = next_token;
    }

    return retval;
}

bool LogModuleEnabled(enum LogModule mod)
{
    assert(mod > LOG_MOD_NONE);
    assert(mod < LOG_MOD_MAX);

    if (module_is_enabled[mod])
    {
        return true;
    }
    else
    {
        return false;
    }
}

void LogDebug(enum LogModule mod, const char *fmt, ...)
{
    assert(mod < LOG_MOD_MAX);

    /* Did we forget any entry in log_modules? */
    nt_static_assert(sizeof(log_modules) / sizeof(log_modules[0]) == LOG_MOD_MAX);

    if (LogModuleEnabled(mod))
    {
        va_list ap;
        va_start(ap, fmt);
        VLog(LOG_LEVEL_DEBUG, fmt, ap);
        va_end(ap);
        /* VLog(LOG_LEVEL_DEBUG, "%s: ...", */
        /*      debug_modules_description[mod_order], ...); */
    }
}


void LogSetGlobalLevel(LogLevel level)
{
    global_level = level;
    if (global_system_log_level == LOG_LEVEL_NOTHING)
    {
        LoggingPrivSetLevels(level, level);
    }
    else
    {
        LoggingPrivSetLevels(global_system_log_level, level);
    }
}

void LogSetGlobalLevelArgOrExit(const char *const arg)
{
    LogLevel level = LogLevelFromString(arg);
    if (level == LOG_LEVEL_NOTHING)
    {
        // This function is used as part of initializing the logging
        // system. Using Log() can be considered incorrect, even though
        // it may "work". Let's just print an error to stderr:
        fprintf(stderr, "Invalid log level: '%s'\n", arg);
        DoCleanupAndExit(1);
    }
    LogSetGlobalLevel(level);
}

LogLevel LogGetGlobalLevel(void)
{
    return global_level;
}

void LogSetGlobalSystemLogLevel(LogLevel level)
{
    /* LOG_LEVEL_NOTHING means "unset" (see LogUnsetGlobalSystemLogLevel()) */
    assert(level != LOG_LEVEL_NOTHING);

    global_system_log_level = level;
    LoggingPrivSetLevels(level, global_level);
}

void LogUnsetGlobalSystemLogLevel(void)
{
    global_system_log_level = LOG_LEVEL_NOTHING;
    LoggingPrivSetLevels(global_level, global_level);
}

LogLevel LogGetGlobalSystemLogLevel(void)
{
    return global_system_log_level;
}

void LoggingSetColor(bool enabled)
{
    LoggingContext *lctx = GetCurrentThreadContext();
    lctx->color = enabled;
}

// byte_magnitude and byte_unit are used to print human readable byte counts
long byte_magnitude(long bytes)
{
    const long Ki = 1024;
    const long Mi = Ki * 1024;
    const long Gi = Mi * 1024;

    if (bytes > 8 * Gi)
    {
        return bytes / Gi;
    }
    else if (bytes > 8 * Mi)
    {
        return bytes / Mi;
    }
    else if (bytes > 8 * Ki)
    {
        return bytes / Ki;
    }
    return bytes;
}

// Use this with byte_magnitude
// Note that the cutoff is at 8x unit, because 3192 bytes is arguably more
// useful than 3KiB
const char *byte_unit(long bytes)
{
    const long Ki = 1024;
    const long Mi = Ki * 1024;
    const long Gi = Mi * 1024;

    if (bytes > 8 * Gi)
    {
        return "GiB";
    }
    else if (bytes > 8 * Mi)
    {
        return "MiB";
    }
    else if (bytes > 8 * Ki)
    {
        return "KiB";
    }
    return "bytes";
}

void LogEntryDestroy(LogEntry *entry)
{
    if (entry != NULL)
    {
        free(entry->msg);
        free(entry);
    }
}

void StartLoggingIntoBuffer(LogLevel min_level, LogLevel max_level)
{
    assert((log_buffer == NULL) && (!logging_into_buffer));

    if (log_buffer != NULL)
    {
        /* Should never happen. */
        Log(LOG_LEVEL_ERR, "Re-initializing log buffer without prior commit, discarding messages");
        DiscardLogBuffer();
    }

    log_buffer = SeqNew(16, LogEntryDestroy);
    logging_into_buffer = true;
    min_log_into_buffer_level = min_level;
    max_log_into_buffer_level = max_level;
}

void DiscardLogBuffer()
{
    SeqDestroy(log_buffer);
    log_buffer = NULL;
    logging_into_buffer = false;
}

void CommitLogBuffer()
{
    assert(logging_into_buffer);
    assert(log_buffer != NULL);

    if (log_buffer == NULL)
    {
        /* Should never happen. */
        Log(LOG_LEVEL_ERR, "Attempt to commit an unitialized log buffer");
    }

    /* Disable now so that LogNonConstant() below doesn't append the message
     * into the buffer instaed of logging it. */
    logging_into_buffer = false;

    const size_t n_entries = SeqLength(log_buffer);
    for (size_t i = 0; i < n_entries; i++)
    {
        LogEntry *entry = SeqAt(log_buffer, i);
        LogNoFormat(entry->level, entry->msg);
    }

    DiscardLogBuffer();
}
