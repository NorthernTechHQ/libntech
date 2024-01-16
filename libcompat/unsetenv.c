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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>

#if !HAVE_DECL_UNSETENV
int unsetenv(const char *name);
#endif

/* Under MinGW putenv('var=') will remove variable from environment */

#ifdef __MINGW32__

int unsetenv(const char *name)
{
    int retval;
    char *buf;

    if (name == NULL || *name == 0 || strchr(name, '=') != 0)
    {
        errno = EINVAL;
        return -1;
    }

    buf = malloc(strlen(name) + 2);

    if (!buf)
    {
        errno = ENOMEM;
        return -1;
    }

    sprintf(buf, "%s=", name);
    retval = putenv(buf);
    free(buf);
    return retval;
}

#endif

/*
 * Under SVR4 (Solaris 8/9, HP-UX 11.11) we need to manually update 'environ'
 * variable
 */

#if defined(__sun) || defined (__hpux)

/*
 * Note: this function will leak memory as we don't know how to free data
 * previously used by environment variables.
 */

extern char **environ;

int unsetenv(const char *name)
{
    char **c;
    int len;

    if (name == NULL || *name == 0 || strchr(name, '=') != 0)
    {
        errno = EINVAL;
        return -1;
    }

    len = strlen(name);

/* Find variable */
    for (c = environ; *c; ++c)
    {
        if (strncmp(name, *c, len) == 0 && ((*c)[len] == '=' || (*c)[len] == 0))
        {
            break;
        }
    }

/* Shift remaining values */
    for (; *c; ++c)
    {
        *c = *(c + 1);
    }

    return 0;
}

#endif
