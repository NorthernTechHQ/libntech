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

#include <platform.h>
#include <known_dirs.h>
#include <definitions.h>
#include <file_lib.h>

static char OVERRIDE_BINDIR[PATH_MAX] = {0};

static const char *GetDefaultDir_helper(char dir[PATH_MAX], const char *root_dir,
                                        const char *append_dir);

#ifdef __MINGW32__

const char *GetDefaultWorkDir(void);
const char *GetDefaultLogDir(void);
const char *GetDefaultDataDir(void);
const char *GetDefaultPidDir(void);
const char *GetDefaultMasterDir(void);
const char *GetDefaultInputDir(void);
const char *GetDefaultModuleDir(void);
const char *GetDefaultKeyDir(void);

#endif

#if defined(__CYGWIN__) || defined(__ANDROID__)

#define GET_DEFAULT_DIRECTORY_DEFINE(FUNC, GLOBAL)  \
static const char *GetDefault##FUNC##Dir(void)      \
{                                                   \
    return GLOBAL;                                  \
}

/* getpwuid() on Android returns /data,
 * so use compile-time default instead */
GET_DEFAULT_DIRECTORY_DEFINE(Work, WORKDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Bin, BINDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Data, CF_DATADIR)
GET_DEFAULT_DIRECTORY_DEFINE(Log, LOGDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Pid, PIDDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Input, INPUTDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Master, MASTERDIR)
GET_DEFAULT_DIRECTORY_DEFINE(State, STATEDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Module, MODULEDIR)
GET_DEFAULT_DIRECTORY_DEFINE(Key, KEYDIR)


#elif !defined(__MINGW32__)

static const char *GetDefaultDir_helper(char dir[PATH_MAX], const char *root_dir,
                                        const char *append_dir)
{
    assert(dir != NULL);

    if (getuid() > 0)
    {
        if (dir[0] == '\0')
        {
            struct passwd *mpw = getpwuid(getuid());

            if (mpw == NULL)
            {
                return NULL;
            }

            if ( append_dir == NULL )
            {
                if (snprintf(dir, PATH_MAX, "%s/.cfagent",
                             mpw->pw_dir) >= PATH_MAX)
                {
                    return NULL;
                }
            }
            else
            {
                if (snprintf(dir, PATH_MAX, "%s/.cfagent/%s",
                             mpw->pw_dir, append_dir) >= PATH_MAX)
                {
                    return NULL;
                }
            }
        }
        return dir;
    }
    else
    {
        return root_dir;
    }
}

#endif

#define GET_DEFAULT_DIRECTORY_DEFINE(FUNC, STATIC, GLOBAL, FOLDER)  \
const char *GetDefault##FUNC##Dir(void)                             \
{                                                                   \
    static char STATIC##dir[PATH_MAX]; /* GLOBAL_C */               \
    return GetDefaultDir_helper(STATIC##dir, GLOBAL, FOLDER);       \
}

#if !defined(__CYGWIN__) && !defined(__ANDROID__)
GET_DEFAULT_DIRECTORY_DEFINE(Work, work, WORKDIR, NULL)
GET_DEFAULT_DIRECTORY_DEFINE(Bin, bin, BINDIR, "bin")
GET_DEFAULT_DIRECTORY_DEFINE(Data, data, CF_DATADIR, "data")
GET_DEFAULT_DIRECTORY_DEFINE(Log, log, LOGDIR, "log")
GET_DEFAULT_DIRECTORY_DEFINE(Pid, pid, PIDDIR, NULL)
GET_DEFAULT_DIRECTORY_DEFINE(Master, master, MASTERDIR, "masterfiles")
GET_DEFAULT_DIRECTORY_DEFINE(Input, input, INPUTDIR, "inputs")
GET_DEFAULT_DIRECTORY_DEFINE(State, state, STATEDIR, "state")
GET_DEFAULT_DIRECTORY_DEFINE(Module, module, MODULEDIR, "modules")
GET_DEFAULT_DIRECTORY_DEFINE(Key, key, KEYDIR, "ppkeys")
#endif


/*******************************************************************/

const char *GetWorkDir(void)
{
    const char *workdir = getenv("CFENGINE_TEST_OVERRIDE_WORKDIR");

    return workdir == NULL ? GetDefaultWorkDir() : workdir;
}

const char *GetBinDir(void)
{
    const char *workdir = getenv("CFENGINE_TEST_OVERRIDE_WORKDIR");

    if (workdir == NULL)
    {
#ifdef __MINGW32__
        /* Compile-time default bindir doesn't work on windows because during
         * the build /var/cfengine/bin is used and only when the package is
         * created, things are shuffled around */
        snprintf(OVERRIDE_BINDIR, PATH_MAX, "%s%cbin",
                 GetDefaultWorkDir(), FILE_SEPARATOR);
        return OVERRIDE_BINDIR;
#else
        return GetDefaultBinDir();
#endif
    }
    else
    {
        snprintf(OVERRIDE_BINDIR, PATH_MAX, "%s%cbin",
                 workdir, FILE_SEPARATOR);
        return OVERRIDE_BINDIR;
    }
}

const char *GetLogDir(void)
{
    const char *logdir = getenv("CFENGINE_TEST_OVERRIDE_WORKDIR");

    return logdir == NULL ? GetDefaultLogDir() : logdir;
}

const char *GetPidDir(void)
{
    const char *piddir = getenv("CFENGINE_TEST_OVERRIDE_WORKDIR");

    return piddir == NULL ? GetDefaultPidDir() : piddir;
}

#define GET_DIRECTORY_DEFINE_FUNC_BODY(FUNC, VAR, GLOBAL, FOLDER)    \
{                                                                    \
    const char *VAR##dir = getenv("CFENGINE_TEST_OVERRIDE_WORKDIR"); \
                                                                     \
    static char workbuf[CF_BUFSIZE];                                 \
                                                                     \
    if (VAR##dir != NULL)                                            \
    {                                                                \
        snprintf(workbuf, CF_BUFSIZE, "%s/" #FOLDER, VAR##dir);      \
    }                                                                \
    else if (strcmp(GLOBAL##DIR, "default") == 0 )                   \
    {                                                                \
        snprintf(workbuf, CF_BUFSIZE, "%s/" #FOLDER, GetWorkDir());  \
    }                                                                \
    else /* VAR##dir defined at compile-time */                      \
    {                                                                \
        return GetDefault##FUNC##Dir();                              \
    }                                                                \
                                                                     \
    return MapName(workbuf);                                         \
}

const char *GetInputDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(Input, input, INPUT, inputs)
const char *GetMasterDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(Master, master, MASTER, masterfiles)
const char *GetModuleDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(Module, module, MODULE, modules)
const char *GetStateDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(State, state, STATE, state)
const char *GetDataDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(Data, data, CF_DATA, data)
const char *GetKeyDir(void)
    GET_DIRECTORY_DEFINE_FUNC_BODY(Key, key, KEY, ppkeys)
