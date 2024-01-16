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

#ifndef CFENGINE_PATH_H
#define CFENGINE_PATH_H

// TODO: Move more functions from files_names.c here

/**
 * @brief Returns the filename part of a string/path, similar to basename
 *
 * Differs from basename() by not modifying the input arg and
 * just returning a pointer within the arg (not an internal static buffer).
 * Locates the string after the last `/`. If none are present, assumes it's
 * a relative path, and that the whole string is a filename.
 *
 * @return Pointer to filename within path, NULL if path ends in / (directory)
 */
const char *Path_Basename(const char *path);

char *Path_JoinAlloc(const char *dir, const char *leaf);

/**
 * Get a quoted path if not already quoted and if needs to be quoted (contains
 * spaces and/or other special characters).
 *
 * @return A newly-allocated string (or %NULL if given %NULL)
 */
char *Path_GetQuoted(const char *path);

#endif
