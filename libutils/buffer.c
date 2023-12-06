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
#include <alloc.h>
#include <buffer.h>
#include <refcount.h>
#include <misc_lib.h>
#ifdef WITH_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif
#include <string_lib.h>
#include <logging.h>

Buffer *BufferNewWithCapacity(size_t initial_capacity)
{
    Buffer *buffer = xmalloc(sizeof(Buffer));

    buffer->capacity = initial_capacity;
    buffer->buffer = xmalloc(buffer->capacity);
    buffer->buffer[0] = '\0';
    buffer->mode = BUFFER_BEHAVIOR_CSTRING;
    buffer->used = 0;

    return buffer;
}

Buffer *BufferNew(void)
{
    return BufferNewWithCapacity(DEFAULT_BUFFER_CAPACITY);
}

static void ExpandIfNeeded(Buffer *buffer, size_t needed)
{
    assert(buffer != NULL);
    if (needed >= buffer->capacity)
    {
        size_t new_capacity = UpperPowerOfTwo(needed + 1);
        buffer->buffer = xrealloc(buffer->buffer, new_capacity);
        buffer->capacity = new_capacity;
    }
}

Buffer* BufferNewFrom(const char *data, size_t length)
{
    Buffer *buffer = BufferNewWithCapacity(length + 1);
    BufferAppend(buffer, data, length);

    return buffer;
}

void BufferDestroy(Buffer *buffer)
{
    if (buffer != NULL)
    {
        free(buffer->buffer);
        free(buffer);
    }
}

char *BufferClose(Buffer *buffer)
{
    assert(buffer != NULL);
    char *detached = buffer->buffer;
    free(buffer);

    return detached;
}

Buffer *BufferCopy(const Buffer *source)
{
    assert(source != NULL);
    return BufferNewFrom(source->buffer, source->used);
}

int BufferCompare(const Buffer *buffer1, const Buffer *buffer2)
{
    assert(buffer1 != NULL);
    assert(buffer2 != NULL);

    /*
     * Rules for comparison:
     * 2. Check the content
     * 2.1. If modes are different, check until the first '\0'
     * 2.2. If sizes are different, check until the first buffer ends.
     */
    if (buffer1->mode == buffer2->mode)
    {
        if (buffer1->mode == BUFFER_BEHAVIOR_CSTRING)
        {
            /*
             * C String comparison
             */
            // Adding this as strcmp gives difference of the buffer values in aarch64
            // Whereas it gives 1, 0 or -1 in other platforms accordingly.
            int compare_result = strcmp(buffer1->buffer, buffer2->buffer);
            if (compare_result != 0)
            {
                  compare_result = compare_result / abs(compare_result);
            }
            return compare_result;
        }
        else
        {
            /*
             * BUFFER_BEHAVIOR_BYTEARRAY
             * Byte by byte comparison
             */
            size_t i = 0;
            if (buffer1->used < buffer2->used)
            {
                for (i = 0; i < buffer1->used; ++i)
                {
                    if (buffer1->buffer[i] < buffer2->buffer[i])
                    {
                        return -1;
                    }
                    else if (buffer1->buffer[i] > buffer2->buffer[i])
                    {
                        return 1;
                    }
                }
                return -1;
            }
            else if (buffer1->used == buffer2->used)
            {
                for (i = 0; i < buffer1->used; ++i)
                {
                    if (buffer1->buffer[i] < buffer2->buffer[i])
                    {
                        return -1;
                    }
                    else if (buffer1->buffer[i] > buffer2->buffer[i])
                    {
                        return 1;
                    }
                }
            }
            else
            {
                for (i = 0; i < buffer2->used; ++i)
                {
                    if (buffer1->buffer[i] < buffer2->buffer[i])
                    {
                        return -1;
                    }
                    else if (buffer1->buffer[i] > buffer2->buffer[i])
                    {
                        return 1;
                    }
                }
                return 1;
            }
        }
    }
    else
    {
        /*
         * Mixed comparison
         * Notice that every BYTEARRAY was born as a CSTRING.
         * When we switch back to CSTRING we adjust the length to
         * match the first '\0'.
         */
        size_t i = 0;
        if (buffer1->used < buffer2->used)
        {
            for (i = 0; i < buffer1->used; ++i)
            {
                if (buffer1->buffer[i] < buffer2->buffer[i])
                {
                    return -1;
                }
                else if (buffer1->buffer[i] > buffer2->buffer[i])
                {
                    return 1;
                }
            }
            return -1;
        }
        else if (buffer1->used == buffer2->used)
        {
            for (i = 0; i < buffer1->used; ++i)
            {
                if (buffer1->buffer[i] < buffer2->buffer[i])
                {
                    return -1;
                }
                else if (buffer1->buffer[i] > buffer2->buffer[i])
                {
                    return 1;
                }
            }
        }
        else
        {
            for (i = 0; i < buffer2->used; ++i)
            {
                if (buffer1->buffer[i] < buffer2->buffer[i])
                {
                    return -1;
                }
                else if (buffer1->buffer[i] > buffer2->buffer[i])
                {
                    return 1;
                }
            }
            return 1;
        }
    }
    /*
     * We did all we could and the buffers seems to be equal.
     */
    return 0;
}

void BufferSet(Buffer *buffer, const char *bytes, size_t length)
{
    assert(buffer != NULL);
    assert(bytes != NULL);

    BufferClear(buffer);

    BufferAppend(buffer, bytes, length);
}

char *BufferGet(Buffer *buffer)
{
    assert(buffer != NULL);
    buffer->unsafe = true;
    return buffer->buffer;
}

void BufferAppendString(Buffer *buffer, const char *str)
{
    assert(buffer != NULL);

    size_t len = strlen(str);
    ExpandIfNeeded(buffer, buffer->used + len + 1);
    memcpy(buffer->buffer + buffer->used, str, len);
    buffer->used += len;
    buffer->buffer[buffer->used] = '\0';
}

void BufferTrimToMaxLength(Buffer *buffer, size_t max)
{
    assert(buffer != NULL);

    if (buffer->used > max)
    {
        buffer->used = max;
        // no need to call ExpandIfNeeded
        buffer->buffer[buffer->used] = '\0';
    }
}

void BufferAppend(Buffer *buffer, const char *bytes, size_t length)
{
    assert(buffer != NULL);
    assert(bytes != NULL);

    if (length == 0)
    {
        return;
    }

    switch (buffer->mode)
    {
    case BUFFER_BEHAVIOR_CSTRING:
        {
            size_t actual_length = strnlen(bytes, length);
            ExpandIfNeeded(buffer, buffer->used + actual_length + 1);
            memcpy(buffer->buffer + buffer->used, bytes, actual_length);
            buffer->used += actual_length;
            buffer->buffer[buffer->used] = '\0';
        }
        break;

    case BUFFER_BEHAVIOR_BYTEARRAY:
        ExpandIfNeeded(buffer, buffer->used + length);
        memcpy(buffer->buffer + buffer->used, bytes, length);
        buffer->used += length;
        break;
    }
}

void BufferAppendChar(Buffer *buffer, char byte)
{
    assert(buffer != NULL);
    if (buffer->used < (buffer->capacity - 1))
    {
        buffer->buffer[buffer->used] = byte;
        buffer->used++;

        if (buffer->mode == BUFFER_BEHAVIOR_CSTRING)
        {
            buffer->buffer[buffer->used] = '\0';
        }
    }
    else
    {
        BufferAppend(buffer, &byte, 1);
    }
}

void BufferAppendF(Buffer *buffer, const char *format, ...)
{
    assert(buffer != NULL);
    assert(format != NULL);

    va_list ap;
    va_list aq;
    va_start(ap, format);
    va_copy(aq, ap);

    int printed = vsnprintf(buffer->buffer + buffer->used, buffer->capacity - buffer->used, format, aq);
    assert(printed >= 0);
    if ((size_t) printed >= (buffer->capacity - buffer->used))
    {
        /*
         * Allocate a larger buffer and retry.
         * Now is when having a copy of the list pays off :-)
         */
        ExpandIfNeeded(buffer, buffer->used + printed);

        printed = vsnprintf(buffer->buffer + buffer->used, buffer->capacity - buffer->used, format, ap);
        buffer->used += printed;
    }
    else
    {
        buffer->used += printed;
    }
    va_end(aq);
    va_end(ap);
}

int BufferPrintf(Buffer *buffer, const char *format, ...)
{
    assert(buffer != NULL);
    assert(format != NULL);
    /*
     * We declare two lists, in case we need to reiterate over the list because the buffer was
     * too small.
     */
    va_list ap;
    va_list aq;
    va_start(ap, format);
    va_copy(aq, ap);

    /*
     * We don't know how big of a buffer we will need. It might be that we have enough space
     * or it might be that we don't have enough space. Unfortunately, we cannot reiterate over
     * a va_list, so our only solution is to tell the caller to retry the call. We signal this
     * by returning zero. Before doing that we increase the buffer to a suitable size.
     * The tricky part is the implicit sharing and the reference counting, if we are not shared then
     * everything is easy, however if we are shared then we need a different strategy.
     */
    int printed = vsnprintf(buffer->buffer, buffer->capacity, format, aq);
    if (printed < 0) 
    {
        // vsnprintf failed!
    }
    else if ((size_t) printed >= buffer->capacity)
    {
        /*
         * Allocate a larger buffer and retry.
         * Now is when having a copy of the list pays off :-)
         */
        ExpandIfNeeded(buffer, printed);

        buffer->used = 0;
        printed = vsnprintf(buffer->buffer, buffer->capacity, format, ap);
        buffer->used = printed;
    }
    else
    {
        buffer->used = printed;
    }
    va_end(aq);
    va_end(ap);
    return printed;
}

// NB! Make sure to sanitize format if taken from user input
int BufferVPrintf(Buffer *buffer, const char *format, va_list ap)
{
    assert(buffer != NULL);
    assert(format != NULL);
    va_list aq;
    va_copy(aq, ap);

    /*
     * We don't know how big of a buffer we will need. It might be that we have enough space
     * or it might be that we don't have enough space. Unfortunately, we cannot reiterate over
     * a va_list, so our only solution is to tell the caller to retry the call. We signal this
     * by returning zero. Before doing that we increase the buffer to a suitable size.
     * The tricky part is the implicit sharing and the reference counting, if we are not shared then
     * everything is easy, however if we are shared then we need a different strategy.
     */

    int printed = vsnprintf(buffer->buffer, buffer->capacity, format, aq);
    if (printed < 0)
    {
        // vsnprintf failed!
    }
    else if ((size_t) printed >= buffer->capacity)
    {
        ExpandIfNeeded(buffer, printed);
        buffer->used = 0;
        printed = vsnprintf(buffer->buffer, buffer->capacity, format, ap);
        buffer->used = printed;
    }
    else
    {
        buffer->used = printed;
    }
    va_end(aq);
    return printed;
}

#ifdef WITH_PCRE2
static char *ExpandCFESpecialReplacements(const pcre2_code *regex,
                                          const char *orig_str, size_t orig_str_len,
                                          pcre2_match_data *md, const char *substitute)
{
    /* pcre2_substitute() supports '$n' backreferences in the substitution
     * string, but not '\n' so we need to translate '\n' into '$n'.
     */
    /* And the regex_replace() CFEngine policy function needs this:
     *   In addition, $+ is replaced with the capture count. $' (dollar sign +
     *   single quote) is the part of the string after the regex match. $`
     *   (dollar sign + backtick) is the part of the string before the regex
     *   match. $& holds the entire regex match.
     *
     * Moreover, according to the function's acceptance tests, backreferences
     * with no respective capture group should be replaced with empty strings.
     */

    bool has_special_seq = false;
    bool has_backslash_bref = false;
    int highest_bref = -1;
    for (const char *c = substitute; *c != '\0'; c++)
    {
        if ((c[0] == '$') &&
            ((c[1] == '+') || (c[1] == '`') || (c[1] == '\'') || (c[1] == '&')))
        {
            has_special_seq = true;
        }
        else if ((c[0] == '\\') && isdigit(c[1]))
        {
            has_backslash_bref = true;
            if ((c[1] - '0') > highest_bref)
            {
                highest_bref = c[1] - '0';
            }
        }
        else if ((c[0] == '$') && isdigit(c[1]) && ((c[1] - '0') > highest_bref))
        {
            highest_bref = c[1] - '0';
        }
    }

    uint32_t n_captures_info = 0;
    NDEBUG_UNUSED int ret = pcre2_pattern_info(regex, PCRE2_INFO_CAPTURECOUNT, &n_captures_info);
    assert(ret == 0); /* can't really fail */

    /* for signed comparisons */
    int n_captures = n_captures_info;
    if (!has_special_seq && !has_backslash_bref && (highest_bref <= n_captures))
    {
        /* nothing to do */
        return NULL;
    }
    /* else we probably need to do some replacements */

    size_t subst_len = strlen(substitute);
    Buffer *new_subst_buf;

    /* First, let's replace the special $-sequences. */
    if (has_special_seq)
    {
        new_subst_buf = BufferNewWithCapacity(subst_len);
        size_t *ovec = pcre2_get_ovector_pointer(md);
        const char *match_start = orig_str + ovec[0];
        const char *match_end = orig_str + ovec[1];

        const char *dollar = strchr(substitute, '$');
        const char *subst_offset = substitute;
        while (dollar != NULL)
        {
            if (dollar[1] == '+')
            {
                BufferAppend(new_subst_buf, subst_offset, dollar - subst_offset);
                BufferAppendF(new_subst_buf, "%"PRIu32, n_captures);
                subst_offset = dollar + 2;
            }
            else if (dollar[1] == '`')
            {
                BufferAppend(new_subst_buf, subst_offset, dollar - subst_offset);
                BufferAppend(new_subst_buf, orig_str, match_start - orig_str);
                subst_offset = dollar + 2;
            }
            else if (dollar[1] == '\'')
            {
                BufferAppend(new_subst_buf, subst_offset, dollar - subst_offset);
                BufferAppend(new_subst_buf, match_end, (orig_str + orig_str_len) - match_end);
                subst_offset = dollar + 2;
            }
            else if (dollar[1] == '&')
            {
                BufferAppend(new_subst_buf, subst_offset, dollar - subst_offset);
                BufferAppend(new_subst_buf, match_start, match_end - match_start);
                subst_offset = dollar + 2;
            }
            /* else a $-sequence we don't care about here */

            dollar = strchr(dollar + 1, '$');
        }
        BufferAppend(new_subst_buf, subst_offset, (substitute + subst_len) - subst_offset);
    }
    else
    {
        new_subst_buf = BufferNewFrom(substitute, subst_len);
    }

    /* Now, let's deal with the \n sequences (if any)*/
    char *buf_data = BufferGet(new_subst_buf);
    if (has_backslash_bref)
    {
        /* start with the first backslash in the substitute copy */
        char *backslash = strchr(buf_data, '\\');
        assert(backslash != NULL); /* must still be there */
        do
        {
            /* it's safe to check the byte after the backslash because it will
             * either be the next character or the NUL byte */
            if (isdigit(backslash[1]))
            {
                backslash[0] = '$';
            }
            backslash = strchr(backslash + 1, '\\');
        } while (backslash != NULL);
    }

    /* Finally, remove the backreferences with no respective capture groups */
    if (highest_bref > n_captures)
    {
        subst_len = BufferSize(new_subst_buf);
        size_t i = 0;

        /* first, skip the part before the first '$' */
        while ((i < subst_len) && (buf_data[i] != '$'))
        {
            i++;
        }
        assert(i < subst_len);  /* there must be a '$' if highest_bref > n_captures */

        /* now, delete all invalid backrefs */
        size_t offset = 0;
        while (i < subst_len)
        {
            if ((buf_data[i] == '$') && isdigit(buf_data[i + 1]) && (atoi(buf_data + i + 1) > n_captures))
            {
                /* skip the '$' */
                i++;
                offset++;
                while (isdigit(buf_data[i]))
                {
                    /* skip the number */
                    i++;
                    offset++;
                }
            }
            else
            {
                /* just copy the character and move to the next one */
                buf_data[i - offset] = buf_data[i];
                i++;
            }
        }
        BufferTrimToMaxLength(new_subst_buf, subst_len - offset);
    }

    return BufferClose(new_subst_buf);
}

// returns NULL on success, otherwise an error string
const char* BufferSearchAndReplace(Buffer *buffer, const char *pattern,
                                   const char *substitute, const char *options)
{
    assert(buffer != NULL);
    assert(pattern != NULL);
    assert(substitute != NULL);
    assert(options != NULL);

    static const char *replacement_error_msg = "regex replacement error (see logs for details)";

    /* no default compile opts */
    uint32_t compile_opts = 0;

    /* ensure we get the needed buffer size | substitute with the first match done */
    uint32_t subst_opts = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_MATCHED;

    for (const char *opt_c = options; *opt_c != '\0'; opt_c++)
    {
        switch(*opt_c)
        {
        case 'i':
            compile_opts |= PCRE2_CASELESS;
            break;
        case 'm':
            compile_opts |= PCRE2_MULTILINE;
            break;
        case 's':
            compile_opts |= PCRE2_DOTALL;
            break;
        case 'x':
            compile_opts |= PCRE2_EXTENDED;
            break;
        case 'U':
            compile_opts |= PCRE2_UNGREEDY;
            break;
        case 'g':
            subst_opts |= PCRE2_SUBSTITUTE_GLOBAL;
            break;
        case 'T':
            subst_opts |= PCRE2_SUBSTITUTE_LITERAL;
            break;
        default:
            assert(false);
            Log(LOG_LEVEL_WARNING, "Unsupported regex option '%c'", *opt_c);
        }
    }

    int err_code;
    PCRE2_SIZE err_offset;
    pcre2_code *regex = pcre2_compile((PCRE2_SPTR) pattern, PCRE2_ZERO_TERMINATED,
                                      compile_opts, &err_code, &err_offset, NULL);
    if (regex == NULL)
    {
        char err_msg[128];
        if (pcre2_get_error_message(err_code, (PCRE2_UCHAR*) err_msg, sizeof(err_msg)) !=
            PCRE2_ERROR_BADDATA)
        {
            Log(LOG_LEVEL_ERR, "Failed to compile regex from pattern '%s': '%s' [offset: %zd]",
                pattern, err_msg, err_offset);
        }
        else
        {
            Log(LOG_LEVEL_ERR, "Failed to compile regex from pattern '%s'", pattern);
        }
        return replacement_error_msg;
    }

    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    int ret = pcre2_match(regex, (PCRE2_SPTR) BufferData(buffer), BufferSize(buffer),
                          0, 0, match_data, NULL);
    assert(ret != 0); /* too small ovector in match_data which should never happen */
    if (ret < -1)
    {
        /* error */
        pcre2_match_data_free(match_data);
        pcre2_code_free(regex);
        char err_msg[128];
        if (pcre2_get_error_message(err_code, (PCRE2_UCHAR*) err_msg, sizeof(err_msg)) !=
            PCRE2_ERROR_BADDATA)
        {
            Log(LOG_LEVEL_ERR, "Match error with pattern '%s' on '%s': '%s'",
                pattern, BufferData(buffer), err_msg);
        }
        else
        {
            Log(LOG_LEVEL_ERR, "Unknown match error with pattern '%s' on '%s'",
                pattern, BufferData(buffer));
        }
        return replacement_error_msg;
    }
    else if (ret == -1)
    {
        /* no match => no change needed in the buffer*/
        pcre2_match_data_free(match_data);
        pcre2_code_free(regex);
        return NULL;
    }
    /* else: some match */

    char *subst_copy = ExpandCFESpecialReplacements(regex, BufferData(buffer), BufferSize(buffer),
                                                    match_data, substitute);
    if (subst_copy != NULL)
    {
        substitute = subst_copy;
    }

    /* We don't know how much space we will need for the result, let's start
     * with twice the size of the input and expand it if needed. */
    bool had_enough_space = false;
    size_t out_size = BufferSize(buffer) * 2;
    char *result = xmalloc(out_size);
    while (!had_enough_space)
    {
        ret = pcre2_substitute(regex, (PCRE2_SPTR) BufferData(buffer), BufferSize(buffer),
                               0, subst_opts, match_data, NULL,
                               (PCRE2_SPTR) substitute, PCRE2_ZERO_TERMINATED,
                               result, &out_size);
        if (ret == PCRE2_ERROR_NOMEMORY)
        {
            result = xrealloc(result, out_size);
        }
        had_enough_space = (ret != PCRE2_ERROR_NOMEMORY);
    }

    if (ret < 0)
    {
        char err_msg[128];
        if (pcre2_get_error_message(ret, (PCRE2_UCHAR*) err_msg, sizeof(err_msg)) !=
            PCRE2_ERROR_BADDATA)
        {
            Log(LOG_LEVEL_ERR,
                "Regular expression replacement error: '%s' (replacing '%s' in '%s')",
                err_msg, substitute, BufferData(buffer));
        }
        else
        {
            Log(LOG_LEVEL_ERR,
                "Unknown regular expression replacement error (replacing '%s' in '%s')",
                substitute, BufferData(buffer));
        }
    }
    BufferSet(buffer, result, out_size);

    pcre2_match_data_free(match_data);
    pcre2_code_free(regex);
    free(result);
    free(subst_copy);

    return (ret >= 0) ? NULL : replacement_error_msg;
}

#endif // WITH_PCRE2

void BufferClear(Buffer *buffer)
{
    assert(buffer != NULL);
    buffer->used = 0;
    buffer->buffer[0] = '\0';
}

size_t BufferSize(const Buffer *buffer)
{
    assert(buffer != NULL);
    return buffer != NULL ? buffer->used : 0;
}

const char *BufferData(const Buffer *buffer)
{
    assert(buffer != NULL);
    return buffer != NULL ? buffer->buffer : NULL;
}

void BufferCanonify(Buffer *buffer)
{
    assert(buffer != NULL);
    if (buffer         != NULL &&
        buffer->buffer != NULL)
    {
        CanonifyNameInPlace(buffer->buffer);
    }
}

BufferBehavior BufferMode(const Buffer *buffer)
{
    assert(buffer != NULL);
    return buffer != NULL ? buffer->mode : BUFFER_BEHAVIOR_BYTEARRAY;
}

void BufferSetMode(Buffer *buffer, BufferBehavior mode)
{
    assert(buffer != NULL);
    assert(mode == BUFFER_BEHAVIOR_CSTRING || mode == BUFFER_BEHAVIOR_BYTEARRAY);
    /*
     * If we switch from BYTEARRAY mode to CSTRING then we need to adjust the
     * length to the first '\0'. This makes our life easier in the long run.
     */
    if (BUFFER_BEHAVIOR_CSTRING == mode)
    {
        for (size_t i = 0; i < buffer->used; ++i)
        {
            if (buffer->buffer[i] == '\0')
            {
                buffer->used = i;
                break;
            }
        }
    }
    buffer->mode = mode;
}

Buffer* BufferFilter(Buffer *buffer, BufferFilterFn filter, const bool invert)
{
    assert(buffer != NULL);

    Buffer *filtered = BufferNew();
    for (size_t i = 0; i < buffer->used; ++i)
    {
        bool test = (*filter)(buffer->buffer[i]);
        if (invert)
        {
            test = !test;
        }

        if (test)
        {
            BufferAppendChar(filtered, buffer->buffer[i]);
        }
    }

    return filtered;
}

void BufferRewrite(Buffer *buffer, BufferFilterFn filter, const bool invert)
{
    assert(buffer != NULL);

    Buffer *rewrite = BufferFilter(buffer, filter, invert);
    BufferSet(buffer, BufferData(rewrite), BufferSize(rewrite));
    BufferDestroy(rewrite);
}

size_t BufferCapacity(const Buffer *buffer)
{
    assert(buffer != NULL);
    return buffer != NULL ? buffer->capacity : 0;
}
