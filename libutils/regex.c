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
#include <regex.h>

#include <alloc.h>
#include <logging.h>
#include <string_lib.h>

#include <buffer.h>

Regex *CompileRegex(const char *pattern)
{
    int err_code;
    size_t err_offset;
    pcre2_code *regex = pcre2_compile((PCRE2_SPTR) pattern, PCRE2_ZERO_TERMINATED,
                                      PCRE2_MULTILINE | PCRE2_DOTALL,
                                      &err_code, &err_offset, NULL);

    if (regex != NULL)
    {
        return regex;
    }

    char err_msg[128];
    if (pcre2_get_error_message(err_code, (PCRE2_UCHAR*) err_msg, sizeof(err_msg)) !=
        PCRE2_ERROR_BADDATA)
    {
        Log(LOG_LEVEL_ERR,
            "Regular expression error: '%s' in expression '%s' (offset: %zd)",
            err_msg, pattern, err_offset);
    }
    else
    {
        Log(LOG_LEVEL_ERR,
            "Unknown regular expression error expression '%s' (offset: %zd)",
            pattern, err_offset);
    }
    return NULL;
}

void RegexDestroy(Regex *regex) {
    pcre2_code_free(regex);
}

bool StringMatchWithPrecompiledRegex(const Regex *regex, const char *str,
                                     size_t *start, size_t *end)
{
    assert(regex != NULL);
    assert(str != NULL);

    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    int result = pcre2_match(regex, (PCRE2_SPTR) str, PCRE2_ZERO_TERMINATED,
                             0, 0, match_data, NULL);

    if (result > 0)
    {
        size_t *ovec = pcre2_get_ovector_pointer(match_data);
        if (start != NULL)
        {
            *start = ovec[0];
        }
        if (end != NULL)
        {
            *end = ovec[1];;
        }
    }
    else
    {
        if (start != NULL)
        {
            *start = 0;
        }
        if (end != NULL)
        {
            *end = 0;
        }
    }

    pcre2_match_data_free(match_data);
    return result > 0;
}

bool StringMatch(const char *pattern, const char *str, size_t *start, size_t *end)
{
    Regex *regex = CompileRegex(pattern);

    if (regex == NULL)
    {
        return false;
    }

    bool ret = StringMatchWithPrecompiledRegex(regex, str, start, end);
    RegexDestroy(regex);
    return ret;
}

bool StringMatchFull(const char *pattern, const char *str)
{
    Regex *regex = CompileRegex(pattern);
    if (regex == NULL)
    {
        return false;
    }

    bool ret = StringMatchFullWithPrecompiledRegex(regex, str);
    RegexDestroy(regex);
    return ret;
}

bool StringMatchFullWithPrecompiledRegex(const Regex *regex, const char *str)
{
    size_t start;
    size_t end;
    if (StringMatchWithPrecompiledRegex(regex, str, &start, &end))
    {
        return (start == (size_t) 0) && (end == strlen(str));
    }
    else
    {
        return false;
    }
}

// Returns a Sequence with Buffer elements.

// If return_names is set, the even positions will be the name or
// number of the capturing group, followed by the captured data in the
// odd positions (so for N captures you can expect 2N elements in the
// Sequence).

// If return_names is not set, only the captured data is returned (so
// for N captures you can expect N elements in the Sequence).
Seq *StringMatchCapturesWithPrecompiledRegex(const Regex *regex, const char *str, const bool return_names)
{
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    int result = pcre2_match(regex, (PCRE2_SPTR) str, PCRE2_ZERO_TERMINATED,
                             0, 0, match_data, NULL);
    /* pcre2_match() returns the highest capture group number + 1, i.e. 1 means
     * a match with 0 capture groups. 0 means the vector of offsets is small,
     * negative numbers are errors (incl. no match). */
    if (result < 1)
    {
        pcre2_match_data_free(match_data);
        return NULL;
    }

    uint32_t captures;
    int res = pcre2_pattern_info(regex, PCRE2_INFO_CAPTURECOUNT, &captures);
    if (res != 0)
    {
        pcre2_match_data_free(match_data);
        return NULL;
    }

    uint32_t namecount = 0;
    res = pcre2_pattern_info(regex, PCRE2_INFO_NAMECOUNT, &namecount);
    assert(res == 0);

    const bool do_named_captures = (namecount > 0 && return_names);

    // Get the table of named captures (see explanation below).
    uint32_t name_entry_size = 0;
    unsigned char *name_table = NULL;
    if (do_named_captures)
    {
        res = pcre2_pattern_info(regex, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);
        assert(res == 0);
        res = pcre2_pattern_info(regex, PCRE2_INFO_NAMETABLE, &name_table);
        assert(res == 0);
    }

    size_t *ovector = pcre2_get_ovector_pointer(match_data);
    Seq *ret = SeqNew(captures + 1, BufferDestroy);
    for (uint32_t i = 0; i <= captures; ++i)
    {
        Buffer *capture = NULL;
        if (do_named_captures)
        {
            /* The map contains $namecount entries each with:
             * - 2 bytes representing the position of the capture group in the offset vector
             * - followed by the name of the named group, NUL-terminated.
             * Each entry is padded to be $name_entry_size bytes big. */
            unsigned char *tabptr = name_table;
            for (uint32_t namepos = 0; namepos < namecount; namepos++)
            {
                uint16_t n = (tabptr[0] << 8) | tabptr[1];
                if (n == i) // We found the position
                {
                    capture = BufferNewFrom((char *)(tabptr + 2), name_entry_size - 3);
                    break;
                }
                tabptr += name_entry_size;
            }
        }

        if (return_names)
        {
            if (capture == NULL)
            {
                capture = BufferNew();
                BufferAppendF(capture, "%"PRIu32, i);
            }

            SeqAppend(ret, capture);
        }

        Buffer *data = BufferNewFrom(str + ovector[2*i],
                                     ovector[2*i + 1] - ovector[2 * i]);
        Log(LOG_LEVEL_DEBUG,
            "StringMatchCaptures: return_names = %d, do_named_captures = %s, offset %d, name '%s', data '%s'",
            return_names, do_named_captures ? "true" : "false", i,
            capture == NULL ? "no_name" : BufferData(capture), BufferData(data));
        SeqAppend(ret, data);
    }

    pcre2_match_data_free(match_data);
    return ret;
}

// Returns a Sequence with Buffer elements.

// If return_names is set, the even positions will be the name or
// number of the capturing group, followed by the captured data in the
// odd positions (so for N captures you can expect 2N elements in the
// Sequence).

// If return_names is not set, only the captured data is returned (so
// for N captures you can expect N elements in the Sequence).

Seq *StringMatchCaptures(const char *pattern, const char *str, const bool return_names)
{
    assert(pattern);
    assert(str);

    int err_code;
    size_t err_offset;
    pcre2_code *regex = pcre2_compile((PCRE2_SPTR) pattern, PCRE2_ZERO_TERMINATED,
                                      PCRE2_MULTILINE | PCRE2_DOTALL,
                                      &err_code, &err_offset, NULL);
    if (regex == NULL)
    {
        return NULL;
    }

    Seq *ret = StringMatchCapturesWithPrecompiledRegex(regex, str, return_names);
    pcre2_code_free(regex);
    return ret;
}

bool CompareStringOrRegex(const char *value, const char *compareTo, bool regex)
{
    if (regex)
    {
        if (!NULL_OR_EMPTY(compareTo) && !StringMatchFull(compareTo, value))
        {
            return false;
        }
    }
    else
    {
        if (!NULL_OR_EMPTY(compareTo)  && strcmp(compareTo, value) != 0)
        {
            return false;
        }
    }
    return true;
}

/*
 * This is a fast partial match function. It checks that the compiled rx matches
 * anywhere inside teststring. It does not allocate or free rx!
 */
bool RegexPartialMatch(const Regex *regex, const char *teststring)
{
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(regex, NULL);
    int rc = pcre2_match(regex, (PCRE2_SPTR) teststring, PCRE2_ZERO_TERMINATED, 0, 0, md, NULL);
    pcre2_match_data_free(md);

    /* pcre2_match() returns the highest capture group number + 1, i.e. 1 means
     * a match with 0 capture groups. 0 means the vector of offsets is small,
     * negative numbers are errors (incl. no match). */
    return rc >= 1;
}
