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

#include <platform.h>
#include <path.h>
#include <string_lib.h>
#include <file_lib.h>

const char *Path_Basename(const char *path)
{
    assert(path != NULL);

    const char *filename = strrchr(path, '/');

    if (filename != NULL)
    {
        filename += 1;
    }
    else
    {
        filename = path;
    }

    if (filename[0] == '\0')
    {
        return NULL;
    }

    return filename;
}

char *Path_JoinAlloc(const char *dir, const char *leaf)
{
    if (StringEndsWith(dir, "/"))
    {
        return StringConcatenate(2, dir, leaf);
    }
    return StringConcatenate(3, dir, "/", leaf);
}

char *Path_GetQuoted(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }

    size_t path_len = strlen(path);
    if ((path[0] == '"') && (path[path_len - 1] == '"'))
    {
        /* already quoted, just duplicate */
        return SafeStringDuplicate(path);
    }

    bool needs_quoting = false;
    for (const char *cp = path; !needs_quoting && (*cp != '\0'); cp++)
    {
        /* let's quote everything that's not just alphanumerics, underscores and
         * dashes */
        needs_quoting = !(((*cp >= 'a') && (*cp <= 'z')) ||
                          ((*cp >= 'A') && (*cp <= 'Z')) ||
                          ((*cp >= '0') && (*cp <= '9')) ||
                          (*cp == '_') || (*cp == '-')   ||
                          IsFileSep(*cp));
    }
    if (needs_quoting)
    {
        return StringConcatenate(3, "\"", path, "\"");
    }
    else
    {
        return SafeStringDuplicate(path);
    }
}
