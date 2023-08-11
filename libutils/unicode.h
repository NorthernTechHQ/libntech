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

#ifndef UNICODE_H
#define UNICODE_H

#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
#include <stdint.h>

/**
 * Dumb conversion from 8-bit strings to 16-bit.
 *
 * Does not take locales or any special characters into account.
 * @param dst The destination string.
 * @param src The source string.
 * @param size The size of dst, in wchars.
 */
void ConvertFromCharToWChar(int16_t *dst, const char *src, size_t size);

/**
 * Dumb conversion from 16-bit strings to 8-bit.
 *
 * Does not take locales or any special characters into account. Since
 * it's possible to lose information this way, this function returns a
 * value indicating whether the conversion was "clean" or whether information
 * was lost.
 * @param dst The destination string.
 * @param src The source string.
 * @param size The size of dst, in bytes.
 * @return Returns true if conversion was successful. Returns false if the
 *         16-bit string could not be converted cleanly to 8-bit. Note that dst
 *         will always contain a valid string.
 */
bool ConvertFromWCharToChar(char *dst, const int16_t *src, size_t size);

#endif // !UNICODE_H
