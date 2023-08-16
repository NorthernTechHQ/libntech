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


#ifndef CFENGINE_REGEX_H
#define CFENGINE_REGEX_H

#include <platform.h>                   /* includes config.h */
#include <stdbool.h> // bool

#ifdef WITH_PCRE
#include <pcre.h>
#else
#error "Regex functionality requires PCRE"
#endif

#include <sequence.h>                                           /* Seq */

#define CFENGINE_REGEX_WHITESPACE_IN_CONTEXTS ".*[_A-Za-z0-9][ \\t]+[_A-Za-z0-9].*"

/* Try to use CompileRegex() and StringMatchWithPrecompiledRegex(). */
pcre *CompileRegex(const char *regex);
bool StringMatch(const char *regex, const char *str, size_t *start, size_t *end);
bool StringMatchWithPrecompiledRegex(pcre *regex, const char *str,
                                     size_t *start, size_t *end);
bool StringMatchFull(const char *regex, const char *str);
bool StringMatchFullWithPrecompiledRegex(pcre *regex, const char *str);
Seq *StringMatchCaptures(const char *regex, const char *str, const bool return_names);
Seq *StringMatchCapturesWithPrecompiledRegex(const pcre *pattern, const char *str, const bool return_names);
bool CompareStringOrRegex(const char *value, const char *compareTo, bool regex);

/* Does not free rx! */
bool RegexPartialMatch(const pcre *rx, const char *teststring);

#endif  /* CFENGINE_REGEX_H */
