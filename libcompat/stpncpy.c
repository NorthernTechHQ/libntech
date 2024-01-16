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
#include <assert.h>
#include <stdlib.h>

#if !HAVE_DECL_STPNCPY
char *stpncpy(char *dst, const char *src, size_t len);
#endif

// Copy at most len bytes, including NUL terminating byte
// if src is too long, don't add terminating NUL byte
// if src is shorter than len, fill remainder with NUL bytes
// return pointer to terminator in dst
// if no terminator, return dst + len (address after last byte written)
//
// This is not the fastest way to implement stpncpy, and it is only
// used where it is missing (mingw/windows)
char *stpncpy(char *dst, const char *src, size_t len)
{
    assert(dst != NULL);
    assert(src != NULL);
    assert(dst != src);

    for (int i = 0; i < len; ++i)
    {
        const char copy_byte = src[i];
        dst[i] = copy_byte;
        if (copy_byte == '\0')
        {
            // Zero fill and return:
            for (int j = i+1; j < len; ++j)
            {
                dst[j] = '\0';
            }
            return dst + i;
        }
    }
    return dst + len;
}
