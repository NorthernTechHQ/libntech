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
#include <proc_keyvalue.h>

typedef struct
{
    void *orig_param;
    KeyNumericValueCallback orig_callback;
} KeyNumericParserInfo;

bool KeyNumericParserCallback(const char *field, const char *value, void *param)
{
    KeyNumericParserInfo *info = param;
    long long numeric_value;

    if (sscanf(value,
#if defined(__MINGW32__)
               "%I64d",
#else
               "%lli",
#endif
               &numeric_value) != 1)
    {
        /* Malformed file */
        return false;
    }

    return (*info->orig_callback) (field, numeric_value, info->orig_param);
}

bool ParseKeyNumericValue(FILE *fd, KeyNumericValueCallback callback, void *param)
{
    KeyNumericParserInfo info = { param, callback };

    return ParseKeyValue(fd, KeyNumericParserCallback, &info);
}

bool ParseKeyValue(FILE *fd, KeyValueCallback callback, void *param)
{
    char buf[1024];

    while (fgets(buf, sizeof(buf), fd))
    {
        char *s = strchr(buf, ':');

        if (!s)
        {
            /* Malformed file */
            return false;
        }

        *s = 0;

        if (!(*callback) (buf, s + 1, param))
        {
            return false;
        }
    }

    return (ferror(fd) == 0);
}
