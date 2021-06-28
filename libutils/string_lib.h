/*
  Copyright 2021 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#ifndef CFENGINE_STRING_LIB_H
#define CFENGINE_STRING_LIB_H

#include <alloc.h>
#include <stdbool.h> // bool
#include <stdint.h>  // int64_t
#include <string.h>  // strstr()
#include <stdarg.h> // va_list
#include <compiler.h>
#include <sequence.h>


typedef struct
{
    const char *data;
    size_t len;
} StringRef;


#define NULL_OR_EMPTY(str)                      \
    ((str == NULL) || (str[0] == '\0'))

#define NOT_NULL_AND_EMPTY(str)                 \
    ((str != NULL) && (str[0] == '\0'))

#define STARTSWITH(str,start)                   \
    (strncmp(str,start,strlen(start)) == 0)

#define SAFENULL(str)                           \
    (str != NULL ? str : "(null)")

#ifndef EMPTY_STRING_TO_NULL
#define EMPTY_STRING_TO_NULL(string) ((SafeStringLength(string) != 0)? string : NULL)
#endif
#ifndef NULL_TO_EMPTY_STRING
#define NULL_TO_EMPTY_STRING(string) (string? string : "")
#endif

unsigned int StringHash        (const char *str, unsigned int seed);
static inline unsigned int StringHash_untyped(const void *str, unsigned int seed)
{
    return StringHash(str, seed);
}

/**
 * @brief Format string like sprintf and return formatted string allocated on
 * heap as a return value.
 *
 * @param format Formatting string

 * @return formatted string (on heap) or NULL in case of error. errno is set in
 * the latter case (see errno codes for sprintf).
 */
char *StringFormat(const char *fmt, ...) FUNC_WARN_UNUSED_RESULT FUNC_ATTR_PRINTF(1, 2);

static inline char ToLower(char ch)
{
    if (isupper((unsigned char) ch))
    {
        return (ch - 'A' + 'a');
    }
    else
    {
        return (ch);
    }
}

static inline char ToUpper(char ch)
{
    if ((isdigit((unsigned char) ch)) || (ispunct((unsigned char) ch)))
    {
        return (ch);
    }

    if (isupper((unsigned char) ch))
    {
        return (ch);
    }
    else
    {
        return (ch - 'a' + 'A');
    }
}

static inline void ToUpperStrInplace(char *str)
{
    for (; *str != '\0'; str++)
    {
        *str = ToUpper(*str);
    }
}

static inline void ToLowerStrInplace(char *str)
{
    for (; *str != '\0'; str++)
    {
        *str = ToLower(*str);
    }
}

int StringDecimalToLong(const char *str, long *value_out) FUNC_WARN_UNUSED_RESULT;

int StringToLong(const char *str, long *value_out) FUNC_WARN_UNUSED_RESULT;
void LogStringToLongError(const char *str_attempted, const char *id, int error_code);
long StringToLongExitOnError(const char *str);
long StringToLongUnsafe(const char *str); // Deprecated, do not use
int StringToUlong(const char *str, unsigned long *value_out) FUNC_WARN_UNUSED_RESULT;
unsigned long StringToUlongExitOnError(const char *str);
int StringToInt64(const char *str, int64_t *value_out);
int64_t StringToInt64ExitOnError(const char *str);

/**
 * @brief Converts a string of numerals in base 10 to a long integer,
 *        uses a default value if errors occur.
 *
 * @see StringToLong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[in] default_return Value to return on error
 * @return Result of conversion (or default_return in case of error)
 */
static inline long StringToLongDefaultOnError(const char *str, long default_return)
{
    assert(str != NULL);
    long result = 0;
    int return_code = StringToLong(str, &result);
    if (return_code != 0)
    {
        // Do not log anything because this can be used frequently
        return default_return;
    }
    return result;
}

/**
 * @brief Converts a string of numerals in base 10 to a unsigned long 
 *        integer, uses a default value if errors occur.
 *
 * @see StringToUlong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[in] default_return Value to return on error
 * @return Result of conversion (or default_return in case of error)
 */
static inline unsigned long StringToUlongDefaultOnError(const char *str, unsigned long default_return)
{
    assert(str != NULL);
    unsigned long result = 0;
    int return_code = StringToUlong(str, &result);
    if (return_code != 0)
    {
        // Do not log anything because this can be used frequently
        return default_return;
    }
    return result;
}

/**
 * @brief Converts a string to int64_t, with a default value in case of errors
 *
 * @see StringToInt64()
 * @see StringToLongDefaultOnError()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[in] default_return Value to return on error
 * @return Result of conversion
 */
static inline int64_t StringToInt64DefaultOnError(const char *str, int64_t default_return)
{
    assert(str != NULL);

    int64_t result;
    const int error_code = StringToInt64(str, &result);
    if (error_code != 0)
    {
        return default_return;
    }
    return result;
}

static inline char *StringFromLong(long number)
{
    char *str = xcalloc(32, sizeof(char));
    snprintf(str, 32, "%ld", number);
    return str;
}

static inline double StringToDouble(const char *str)
{
    assert(str);

    char *end;
    double result = strtod(str, &end);

    assert(!*end && "Failed to convert string to double");

    return result;
}

static inline char *StringFromDouble(double number)
{
    return StringFormat("%.2f", number);
}

static inline char *NULLStringToEmpty(char *str)
{
    if(!str)
    {
        return "";
    }

    return str;
}

static inline bool StringIsNumeric(const char *s)
{
    for (; *s; s++)
    {
        if (!isdigit((unsigned char)*s))
        {
            return false;
        }
    }

    return true;
}

static inline bool StringIsPrintable(const char *s)
{
    for (; *s; s++)
    {
        if (!isprint((unsigned char)*s))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Check if a char is "printable", replacement for isprint
 *
 * isprint takes a (signed) int, so if you send in a (signed) char there is an
 * implicit cast. This can be problematic, since 0xF0 becomes 0xFFFFF0.
 *
 * Additionally, at least on HPUX, isprint returns true for some values above
 * 127, depending on locale.
 *
 * Thus, this replacement works the same everywhere, and can be used when you
 * want predictable results, even though it will return false for some values
 * which might be possible to print in your environment / locale.
 *
 * @param c [in] Character to evaluate
 * @return True if c is part of the printable ascii range
 */
static inline bool CharIsPrintableAscii(const char c)
{
    return (c >= ' ' && c <= '~');
}

static inline bool EmptyString(const char *s)
{
    const char *sp;

    for (sp = s; *sp != '\0'; sp++)
    {
        if (!isspace((unsigned char)*sp))
        {
            return false;
        }
    }

    return true;
}

static inline bool StringContains(const char *const haystack, const char *const needle)
{
    return (strstr(haystack, needle) != NULL);
}

static inline bool StringContainsChar(const char *const haystack, const char needle)
{
    return (strchr(haystack, needle) != NULL);
}

size_t StringBytesToHex(char *dst, size_t dst_size,
                        const unsigned char *src_bytes, size_t src_len);

static inline char *SafeStringDuplicate(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    return xstrdup(str);
}

static inline char *SafeStringNDuplicate(const char *str, size_t size)
{
    if (str == NULL)
    {
        return NULL;
    }

    return xstrndup(str, size);
}

static inline int SafeStringLength(const char *str)
{
    if (str == NULL)
    {
        return 0;
    }

    return strlen(str);
}

int StringSafeCompare(const char *a, const char *b);
static inline bool StringEqual(const char *const a, const char *const b)
{
    return (StringSafeCompare(a, b) == 0);
}

int StringSafeCompareN(const char *a, const char *b, size_t n);
static inline bool StringEqualN(const char *const a, const char *const b, const size_t n)
{
    return (StringSafeCompareN(a, b, n) == 0);
}

int StringSafeCompare_IgnoreCase(const char *a, const char *b);
static inline bool StringEqual_IgnoreCase(const char *const a, const char *const b)
{
    return (StringSafeCompare_IgnoreCase(a, b) == 0);
}

int StringSafeCompareN_IgnoreCase(const char *a, const char *b, size_t n);
static inline bool StringEqualN_IgnoreCase(const char *const a, const char *const b, const size_t n)
{
    return (StringSafeCompareN_IgnoreCase(a, b, n) == 0);
}

static inline bool StringEqual_untyped(const void *a, const void *b)
{
    return StringEqual(a, b);
}

char *StringConcatenate(size_t count, const char *first, ...);
char *StringSubstring(const char *source, size_t source_len, int start, int len);

/* Allocates the result */
char *SearchAndReplace(const char *source, const char *search, const char *replace);

ssize_t StringReplace(char *buf, size_t buf_size, const char *find, const char *replace);

static inline bool IsStrIn(const char *str, const char *const strs[])
{
    int i;

    for (i = 0; strs[i]; ++i)
    {
        if (strcmp(str, strs[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

static inline bool IsStrCaseIn(const char *str, const char *const strs[])
{
    int i;

    for (i = 0; strs[i]; ++i)
    {
        if (strcasecmp(str, strs[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

size_t StringCountTokens(const char *str, size_t len, const char *seps);
StringRef StringGetToken(const char *str, size_t len, size_t index, const char *seps);

char **String2StringArray(const char *str, char separator);
void FreeStringArray(char **strs);

int CountChar(const char *string, char sp);
void ReplaceChar(const char *in, char *out, int outSz, char from, char to);

static inline void ReplaceTrailingChar(char *str, char from, char to)
/* Replaces any unwanted last char in str. */
{
    int strLen;

    strLen = SafeStringLength(str);

    if (strLen == 0)
    {
        return;
    }

    if (str[strLen - 1] == from)
    {
        str[strLen - 1] = to;
    }
}

char *EscapeCharCopy(const char *str, char to_escape, char escape_with);

static inline char *ScanPastChars(char *scanpast, char *input)
{
    char *pos = input;

    while ((*pos != '\0') && (strchr(scanpast, *pos)))
    {
        pos++;
    }

    return pos;
}

/**
 * @brief Strips the newline character off a string, in place
 * @param str The string to strip
 * @param max_length Maximum length of input string
 * @return 0 if successful, -1 if the input string was longer than allowed (max_length).
 */
int StripTrailingNewline(char *str, size_t max_length);

/**
 * @brief Remove trailing spaces
 * @param str
 * @param max_length Maximum length of input string
 * @return 0 if successful, -1 if Chop was called on a string that seemed to have no terminator
 */
int Chop(char *str, size_t max_length);

char *TrimWhitespace(char *s);
size_t TrimCSVLineCRLF(char *data);
size_t TrimCSVLineCRLFStrict(char *data);

/**
 * @brief Erase part of a string using memmove()
 *
 * String must be NUL-terminated. Indices from start, up to but not including
 * end are erased. The number of bytes erased is end - start. If start == end,
 * no bytes are erased. Assumes: 0 <= start <= end <= strlen(s)
 */
static inline void StringCloseHole(char *s, const size_t start, const size_t end)
{
    assert(s != NULL);
    assert(start <= end && end <= strlen(s));
    assert((end - start) <= strlen(s));

    if (end > start)
    {
        memmove(s + start, s + end,
                /* The 1+ ensures we copy the final '\0' */
                strlen(s + end) + 1);
    }
}

/**
 * @brief Check if a string ends with the given suffix
 * @param str
 * @param suffix
 * @param case_fold whether the comparison is case-insensitive
 * @return True if suffix matches
 */
bool StringEndsWithCase(const char *str, const char *suffix, const bool case_fold);

/**
 * @brief Check if a string ends with the given suffix
 * @param str
 * @param suffix
 * @return True if suffix matches
 */
static inline bool StringEndsWith(const char *str, const char *suffix)
{
    return StringEndsWithCase(str, suffix, false);
}

/**
 * @brief Check if a string starts with the given prefix
 * @param str
 * @param prefix
 * @return True if prefix matches
 */
bool StringStartsWith(const char *str, const char *prefix);

/**
 * @brief Format string like vsprintf and return formatted string allocated
 * on heap as a return value.
 */
static inline char *StringVFormat(const char *fmt, va_list ap)
{
    char *value;
    int ret = xvasprintf(&value, fmt, ap);
    if (ret < 0)
    {
        return NULL;
    }
    else
    {
        return value;
    }
}



/**
 * @brief Copy a string from `from` to `to` (a buffer of at least buf_size)
 *
 * The destination (`to`) is guaranteed to be NUL terminated,
 * `to[buf_size-1]` will always be '\0'. If the string is shorter
 * the additional trailing bytes are also zeroed.
 *
 * The source (`from`) must either be NUL terminated or big enough
 * to read buf_size characters, even though only buf_size - 1 of them
 * end up in the output buffer.
 *
 * The return value is equal to `strlen(to)`, for large data sizes this
 * is useful since it doesn't require a second pass through the data.
 * The return value should be checked, if it is >= buf_size, it means
 * the string was truncated because it was too long. Expressed in another
 * way, the return value is the number of bytes read from the input (`from`)
 * excluding the terminating `\0` byte (up to buf_size characters will be
 * read).
 *
 * @note `from` and `to` must not overlap
 * @warning Regardless of `strlen(from)`, `to` must be at least `buf_size` big
 * @param[in] from String to copy from, up to buf_size bytes are read
 * @param[out] to Output buffer (minimum buf_size), always '\0' terminated
 * @param[in] buf_size Maximum buffer size to write (including '\0' byte)
 * @return String length of `to`, or `buf_size` in case of overflow
 */
static inline size_t StringCopy(const char *from, char *to, size_t buf_size)
{
    assert(from != NULL);
    assert(to != NULL);
    assert(from != to);

    memset(to, 0, buf_size);
    strncpy(to, from, buf_size);
    if (to[buf_size-1] != '\0')
    {
        to[buf_size-1] = '\0';
        return buf_size;
    }
    return strlen(to); // TODO - Replace the extra pass by using stpncpy:

    /*
    // stpncpy has bad/unsafe behavior when string is too long:
    // * Does not NUL terminate
    // * Returns a pointer to potentially invalid memory
    // These issues have to be handled (even for correct arguments)

    const char *const end = stpncpy(to, from, buf_size);
    assert(end >= to);
    const long len = end - to;
    to[buf_size] = '\0';
    return len;
    */
}

/**
 * Returns pointer to the first byte in #buf that is not #c. Returns NULL if
 * all of #buf contains only bytes of value #c.
 *
 * @NOTE this functions complements memchr() from POSIX.
 *
 * @TODO move to libcompat, it appears to be available in some systems.
 */
static inline void *memcchr(const void *buf, int c, size_t buf_size)
{
    const char *cbuf = buf;
    for (size_t i = 0; i < buf_size; i++)
    {
        if (cbuf[i] != c)
        {
            return (void *) &cbuf[i];                    /* cast-away const */
        }
    }
    return NULL;
}

bool StringNotMatchingSetCapped(const char *isp, size_t limit,
                      const char *exclude, char *obuf);

/**
 * @brief Appends src to dst, but will not exceed n bytes in dst, including the terminating null.
 * @param dst Destination string.
 * @param src Source string.
 * @param n Total size of dst buffer. The string will be truncated if this is exceeded.
 * @return True if append was successful, false if the operation caused an overflow.
 */
static inline bool StringAppend(char *dst, const char *src, size_t n)
{
    size_t i, j;
    n--;
    for (i = 0; i < n && dst[i]; i++)
    {
    }
    for (j = 0; i < n && src[j]; i++, j++)
    {
        dst[i] = src[j];
    }
    dst[i] = '\0';
    return (i < n || !src[j]);
}

char *StringCanonify(char *dst, const char *src);
bool PathAppend(char *path, size_t path_size, const char *leaf, char sep);

void StrCat(char *dst, size_t dst_size, size_t *dst_len,
            const char *src, size_t src_len);
void StrCatDelim(char *dst, size_t dst_size, size_t *dst_len,
                 const char *src, char sep);

static inline void CanonifyNameInPlace(char *s)
{
    for (; *s != '\0'; s++)
    {
        if (!isalnum((unsigned char) *s))
        {
            *s = '_';
        }
    }
}

/**
 * @brief Check if a command line argument matches a short or long option
 *
 * Useful for smaller binaries, when getopt seems overkill and you just want
 * something simple. A match means it's either equal to shortopt or equal to
 * (the first part of) longopt. shortopt is compared case sensitively,
 * while longopt is compared case insensitively.
 *
 * As an example, `--INFO` would match the longopt `--inform`.
 *
 * It doesn't (cannot) detect ambiguities when there are multiple options.
 * In many cases, where you need more flexibility and consistency, it's better
 * to use getopt or something similar.
 *
 * @param[in] supplied User supplied argument from command line (trimmed word)
 * @param[out] longopt Long option alternative to check (ex. "--help")
 * @param[out] shortopt Short option alternative to check (ex. "-H")
 * @return True if `supplied` _matches_ `longopt` or `shortopt`
 */
static inline bool StringMatchesOption(
    const char *const supplied,
    const char *const longopt,
    const char *const shortopt)
{
    assert(supplied != NULL);
    assert(shortopt != NULL);
    assert(longopt != NULL);
    assert(strlen(shortopt) == 2);
    assert(strlen(longopt) >= 3);
    assert(shortopt[0] == '-' && shortopt[1] != '-');
    assert(longopt[0] == '-' && longopt[1] == '-' && longopt[2] != '-');

    const size_t length = strlen(supplied);
    if (length <= 1)
    {
        return false;
    }
    else if (length == 2)
    {
        return StringEqual(supplied, shortopt);
    }
    return StringEqualN_IgnoreCase(supplied, longopt, length);
}

#endif
