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
#include <compiler.h>

#define MAXTRY 999999

char *mkdtemp(char *template)
{
    char *xxx = strrstr(template, "XXXXXX");

    if (xxx == NULL || strcmp(xxx, "XXXXXX") != 0)
    {
        errno = EINVAL;
        return NULL;
    }

    for (int i = 0; i <= MAXTRY; ++i)
    {
        snprintf(xxx, 7, "%06d", i);

        int fd = mkdir(template, S_IRUSR | S_IWUSR | S_IXUSR);
        if (fd >= 0)
        {
            close(fd);
            return template;
        }

        if (errno != EEXIST)
        {
            return NULL;
        }
    }

    errno = EEXIST;
    return NULL;
}
