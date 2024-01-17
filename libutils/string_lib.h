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

#ifndef CFENGINE_STRING_LIB_H
#define CFENGINE_STRING_LIB_H

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
unsigned int StringHash_untyped(const void *str, unsigned int seed);

char ToLower(char ch);
char ToUpper(char ch);
void ToUpperStrInplace(char *str);
void ToLowerStrInplace(char *str);

int StringDecimalToLong(const char *str, long *value_out) FUNC_WARN_UNUSED_RESULT;

int StringToLong(const char *str, long *value_out) FUNC_WARN_UNUSED_RESULT;
void LogStringToLongError(const char *str_attempted, const char *id, int error_code);
long StringToLongDefaultOnError(const char *str, long default_return);
long StringToLongExitOnError(const char *str);
long StringToLongUnsafe(const char *str); // Deprecated, do not use
int StringToUlong(const char *str, unsigned long *value_out) FUNC_WARN_UNUSED_RESULT;
unsigned long StringToUlongDefaultOnError(const char *str, unsigned long default_return);
unsigned long StringToUlongExitOnError(const char *str);
int StringToInt64(const char *str, int64_t *value_out);
int64_t StringToInt64DefaultOnError(const char *str, int64_t default_return);
int64_t StringToInt64ExitOnError(const char *str);

char *StringFromLong(long number);
double StringToDouble(const char *str);
char *StringFromDouble(double number);
char *NULLStringToEmpty(char *str);

bool StringIsNumeric(const char *name);
bool StringIsPrintable(const char *name);

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
bool EmptyString(const char *s);

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

char *SafeStringDuplicate(const char *str);
char *SafeStringNDuplicate(const char *str, size_t size);
int SafeStringLength(const char *str);

int  StringSafeCompare(const char *a, const char *b);
bool StringEqual  (const char *a, const char *b);

static inline bool IsStringInArray(
    const char *const str, const char *const *const array, const size_t n_items)
{
    for (size_t i = 0; i < n_items; i++)
    {
        if (StringEqual(array[i], str))
        {
            return true;
        }
    }
    return false;
}

int  StringSafeCompareN(const char *a, const char *b, size_t n);
bool StringEqualN  (const char *a, const char *b, size_t n);

int  StringSafeCompare_IgnoreCase(const char *a, const char *b);
bool StringEqual_IgnoreCase  (const char *a, const char *b);

int  StringSafeCompareN_IgnoreCase(const char *a, const char *b, size_t n);
bool StringEqualN_IgnoreCase  (const char *a, const char *b, size_t n);

bool StringEqual_untyped(const void *a, const void *b);

char *StringConcatenate(size_t count, const char *first, ...);
char *StringSubstring(const char *source, size_t source_len, int start, int len);

/* Allocates the result */
char *SearchAndReplace(const char *source, const char *search, const char *replace);

ssize_t StringReplace(char *buf, size_t buf_size, const char *find, const char *replace);
ssize_t StringReplaceN(char *buf, size_t buf_size,
                       const char *find, const char *replace, size_t n);

bool IsStrIn(const char *str, const char *const strs[]);
bool IsStrCaseIn(const char *str, const char *const strs[]);

size_t StringCountTokens(const char *str, size_t len, const char *seps);
StringRef StringGetToken(const char *str, size_t len, size_t index, const char *seps);

char **String2StringArray(const char *str, char separator);
void FreeStringArray(char **strs);

int CountChar(const char *string, char sp);
void ReplaceChar(const char *in, char *out, int outSz, char from, char to);
void ReplaceTrailingChar(char *str, char from, char to);
char *EscapeCharCopy(const char *str, char to_escape, char escape_with);

char *ScanPastChars(char *scanpast, char *input);

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
void StringCloseHole(char *s, size_t start, size_t end);

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
bool StringEndsWith(const char *str, const char *suffix);

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
char *StringVFormat(const char *fmt, va_list ap);

/**
 * @brief Format string like sprintf and return formatted string allocated on
 * heap as a return value.
 *
 * @param format Formatting string

 * @return formatted string (on heap) or NULL in case of error. errno is set in
 * the latter case (see errno codes for sprintf).
 */
char *StringFormat(const char *fmt, ...) FUNC_WARN_UNUSED_RESULT FUNC_ATTR_PRINTF(1, 2);

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
size_t StringCopy(const char *from, char *to, size_t buf_size);

void *memcchr(const void *buf, int c, size_t buf_size);

bool StringNotMatchingSetCapped(const char *isp, size_t limit,
                      const char *exclude, char *obuf);

/**
 * @brief Appends src to dst, but will not exceed n bytes in dst, including the terminating null.
 * @param dst Destination string.
 * @param src Source string.
 * @param n Total size of dst buffer. The string will be truncated if this is exceeded.
 * @return True if append was successful, false if the operation caused an overflow.
 */
bool StringAppend(char *dst, const char *src, size_t n);

char *StringCanonify(char *dst, const char *src);
bool PathAppend(char *path, size_t path_size, const char *leaf, char sep);

void StrCat(char *dst, size_t dst_size, size_t *dst_len,
            const char *src, size_t src_len);
void StrCatDelim(char *dst, size_t dst_size, size_t *dst_len,
                 const char *src, char sep);

void CanonifyNameInPlace(char *str);

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
bool StringMatchesOption(
    const char *supplied, const char *longopt, const char *shortopt);

/**
 * @brief Join elements in sequence into a string
 * @param[in] seq Sequence of strings to join
 * @param[in] sep Separator between elements (can be NULL)
 * @return The concatenation of the elements in sequence
 * @note Sequence must contain only NUL-terminated strings, otherwise behavior
 *       is undefined
 */
char *StringJoin(const Seq *seq, const char *sep);

#endif
