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

#include <unicode.h>

void ConvertFromCharToWChar(int16_t *dst, const char *src, size_t size)
{
    size_t c;
    size--; // Room for '\0'.
    for (c = 0; c < size && src[c]; c++)
    {
        dst[c] = (int16_t)src[c];
    }
    dst[c] = '\0';
}

bool ConvertFromWCharToChar(char *dst, const int16_t *src, size_t size)
{
    bool clean = true;
    size_t c;
    size--; // Room for '\0'.
    for (c = 0; c < size && src[c]; c++)
    {
        // We only consider unsigned values.
        if (src[c] < 0 || src[c] >= 0x100)
        {
            clean = false;
            dst[c] = '_';
        }
        else
        {
            dst[c] = (char)src[c];
        }
    }
    dst[c] = '\0';
    return clean;
}
