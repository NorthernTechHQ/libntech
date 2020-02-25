/*
  Copyright 2020 Northern.tech AS

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

#include <platform.h>
#include <string.h>
#include <logging.h>

#ifndef __MINGW32__

#include <sys/types.h>
#include <pwd.h>

/* Max size of the 'passwd' string in the getpwuid_r() function,
 * man:getpwuid_r(3) says that this value "Should be more than enough". */
#define GETPW_R_SIZE_MAX 16384

bool GetCurrentUserName(char *userName, int userNameLen)
{
    char buf[GETPW_R_SIZE_MAX] = {0};
    struct passwd pwd;
    struct passwd *result;

    memset(userName, 0, userNameLen);
    int ret = getpwuid_r(getuid(), &pwd, buf, GETPW_R_SIZE_MAX, &result);

    if (result == NULL)
    {
        Log(LOG_LEVEL_ERR, "Could not get user name of current process, using 'UNKNOWN'. (getpwuid: %s)",
            ret == 0 ? "not found" : GetErrorStrFromCode(ret));
        strlcpy(userName, "UNKNOWN", userNameLen);
        return false;
    }

    strlcpy(userName, result->pw_name, userNameLen);
    return true;
}

#else  /* !__MINGW32__ */

/*****************************************************************************/

#include <windows.h>

/* Get user name of owner of this process */
bool GetCurrentUserName(char *userName, int userNameLen)
{
    DWORD userNameMax = (DWORD) userNameLen;

    if (!GetUserName(userName, &userNameMax))
    {
        Log(LOG_LEVEL_ERR, "Could not get user name of current process, using 'UNKNOWN. (GetUserName)");

        strncpy(userName, "UNKNOWN", userNameLen);
        userName[userNameLen - 1] = '\0';
        return false;
    }

    return true;
}

#endif  /* !__MINGW32__ */
