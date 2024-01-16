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

#if !HAVE_DECL_STRCASECMP
int strncasecmp(const char *s1, const char *s2);
#endif

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0)
    {
        if (*s1 == '\0' && *s2 == '\0')
        {
            break;
        }
        if (*s1 == '\0')
        {
            return -1;
        }
        if (*s2 == '\0')
        {
            return 1;
        }
        if (tolower(*s1) < tolower(*s2))
        {
            return -1;
        }
        if (tolower(*s1) > tolower(*s2))
        {
            return 1;
        }
        s1++;
        s2++;
    }
    return 0;
}
