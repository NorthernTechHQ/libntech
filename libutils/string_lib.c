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
#include <string_lib.h>

#include <alloc.h>
#include <writer.h>
#include <misc_lib.h>
#include <logging.h>
#include <cleanup.h>
#include <definitions.h> // CF_BUFSIZE
#include <condition_macros.h> // nt_static_assert()
#include <printsize.h>

char *StringVFormat(const char *fmt, va_list ap)
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

char *StringFormat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *res = StringVFormat(fmt, ap);
    va_end(ap);
    return res;
}

size_t StringCopy(const char *const from, char *const to, const size_t buf_size)
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

unsigned int StringHash(const char *str, unsigned int seed)
{
    assert(str != NULL);
    unsigned const char *p = (unsigned const char *) str;
    unsigned int h = seed;

    // NULL is not allowed, but we will prevent segfault anyway:
    size_t len = (str != NULL) ? strlen(str) : 0;

    /* https://en.wikipedia.org/wiki/Jenkins_hash_function#one-at-a-time */
    for (size_t i = 0; i < len; i++)
    {
        h += p[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h;
}

unsigned int StringHash_untyped(const void *str, unsigned int seed)
{
    return StringHash(str, seed);
}

char ToLower(char ch)
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

/*********************************************************************/

char ToUpper(char ch)
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

/*********************************************************************/

void ToUpperStrInplace(char *str)
{
    for (; *str != '\0'; str++)
    {
        *str = ToUpper(*str);
    }
}

/*********************************************************************/

void ToLowerStrInplace(char *str)
{
    for (; *str != '\0'; str++)
    {
        *str = ToLower(*str);
    }
}

/*********************************************************************/

char *SafeStringDuplicate(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    return xstrdup(str);
}

/*********************************************************************/

char *SafeStringNDuplicate(const char *str, size_t size)
{
    if (str == NULL)
    {
        return NULL;
    }

    return xstrndup(str, size);
}

/*********************************************************************/

int SafeStringLength(const char *str)
{
    if (str == NULL)
    {
        return 0;
    }

    return strlen(str);
}

// Compare two pointers (strings) where exactly one is NULL
static int NullCompare(const void *const a, const void *const b)
{
    assert(a != b);
    assert(a == NULL || b == NULL);

    if (a != NULL)
    {
        return 1;
    }
    if (b != NULL)
    {
        return -1;
    }

    // Should never happen
    ProgrammingError("Programming Error: NullCompare failed");
    return 101;
}

int StringSafeCompare(const char *const a, const char *const b)
{
    if (a == b) // Same address or both NULL
    {
        return 0;
    }
    if (a != NULL && b != NULL)
    {
        // Adding this as strcmp gives difference of the buffer values in aarch64
        // Whereas it gives 1, 0 or -1 in other platforms accordingly.
        int compare_result = strcmp(a, b);
        if (compare_result != 0)
        {
            compare_result = compare_result / abs(compare_result);
        }
        return compare_result;
    }

    // Weird edge cases where one is NULL:
    return NullCompare(a, b);
}

int StringSafeCompareN(const char *const a, const char *const b, const size_t n)
{
    if (a == b) // Same address or both NULL
    {
        return 0;
    }
    if (a != NULL && b != NULL)
    {
        return strncmp(a, b, n);
    }

    // Weird edge cases where one is NULL:
    return NullCompare(a, b);
}

bool StringEqual(const char *const a, const char *const b)
{
    return (StringSafeCompare(a, b) == 0);
}

bool StringEqualN(const char *const a, const char *const b, const size_t n)
{
    return (StringSafeCompareN(a, b, n) == 0);
}

int StringSafeCompare_IgnoreCase(const char *const a, const char *const b)
{
    if (a == b) // Same address or both NULL
    {
        return 0;
    }
    if (a != NULL && b != NULL)
    {
        return strcasecmp(a, b);
    }

    // Weird edge cases where one is NULL:
    return NullCompare(a, b);
}

int StringSafeCompareN_IgnoreCase(const char *const a, const char *const b, const size_t n)
{
    if (a == b) // Same address or both NULL
    {
        return 0;
    }
    if (a != NULL && b != NULL)
    {
        return strncasecmp(a, b, n);
    }

    // Weird edge cases where one is NULL:
    return NullCompare(a, b);
}

bool StringEqual_IgnoreCase(const char *const a, const char *const b)
{
    return (StringSafeCompare_IgnoreCase(a, b) == 0);
}

bool StringEqualN_IgnoreCase(const char *const a, const char *const b, const size_t n)
{
    return (StringSafeCompareN_IgnoreCase(a, b, n) == 0);
}

bool StringEqual_untyped(const void *a, const void *b)
{
    return StringEqual(a, b);
}

/*********************************************************************/

char *SearchAndReplace(const char *source, const char *search, const char *replace)
{
    const char *source_ptr = source;

    if ((source == NULL) || (search == NULL) || (replace == NULL))
    {
        ProgrammingError("Programming error: NULL argument is passed to SearchAndReplace");
    }

    if (strcmp(search, "") == 0)
    {
        return xstrdup(source);
    }

    Writer *w = StringWriter();

    for (;;)
    {
        const char *found_ptr = strstr(source_ptr, search);

        if (found_ptr == NULL)
        {
            WriterWrite(w, source_ptr);
            return StringWriterClose(w);
        }

        WriterWriteLen(w, source_ptr, found_ptr - source_ptr);
        WriterWrite(w, replace);

        source_ptr += found_ptr - source_ptr + strlen(search);
    }
}

/*********************************************************************/

char *StringConcatenate(size_t count, const char *first, ...)
{
    if (count < 1)
    {
        return NULL;
    }

    size_t total_length = first ? strlen(first) : 0;

    va_list args;
    va_start(args, first);
    for (size_t i = 1; i < count; i++)
    {
        const char *arg = va_arg(args, const char*);
        if (arg)
        {
            total_length += strlen(arg);
        }
    }
    va_end(args);

    char *result = xcalloc(total_length + 1, sizeof(char));
    if (first)
    {
        strncpy(result, first, total_length);
    }

    va_start(args, first);
    for (size_t i = 1; i < count; i++)
    {
        const char *arg = va_arg(args, const char *);
        if (arg)
        {
            strcat(result, arg);
        }
    }
    va_end(args);

    return result;
}

/*********************************************************************/

char *StringSubstring(const char *source, size_t source_len, int start, int len)
{
    ssize_t end = -1;

    if (len == 0)
    {
        return SafeStringDuplicate("");
    }
    else if (len < 0)
    {
        end = source_len + len - 1;
    }
    else
    {
        end = start + len - 1;
    }

    end = MIN(end, source_len - 1);

    if (start < 0)
    {
        start = source_len + start;
    }

    if (start >= end)
    {
        return NULL;
    }

    char *result = xcalloc(end - start + 2, sizeof(char));

    memcpy(result, source + start, end - start + 1);
    return result;
}

/*********************************************************************/

bool StringIsNumeric(const char *s)
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

bool StringIsPrintable(const char *s)
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

bool EmptyString(const char *s)
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

/*********************************************************************/

/**
 * @brief Converts a string of numerals in base 10 to a long integer.
 *
 * Result is stored in *value_out, return value should be checked.
 * On error *value_out is unmodified and an error code is returned.
 * Leading spaces in input string are skipped.
 * String numeral must be terminated by NULL byte or space (isspace).
 *
 * @see StringToLongExitOnError()
 * @see StringToLongDefaultOnError()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[out] value_out Where to store result on success, cannot be NULL
 * @return 0 on success, error code otherwise (see source code)
 */
int StringToLong(const char *str, long *value_out)
{
    nt_static_assert(ERANGE != 0);
    assert(str != NULL);
    assert(value_out != NULL);

    char *endptr = NULL;

    errno = 0;
    const long val = strtol(str, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)))
    {
        return ERANGE; // Overflow or underflow
    }

    if (endptr == str)
    {
        return -81; // No digits found
    }

    if (endptr == NULL)
    {
        return -82; // endpointer not set by strtol
    }

    if (*endptr != '\0' && !isspace(*endptr))
    {
        return -83; // string not properly terminated
    }

    if (errno != 0)
    {
        return errno; // Unknown error
    }

    *value_out = val;
    return 0;
}

/**
 * @brief Convert a string representing a real number to int, discarding the decimal part.
 *
 * Attempts to find the first '.' character in order to copy the integer part to a
 * temporary array before passing it to StringToLong. Should the input string be
 * an entire integer by itself no copying occurs. The integer part must NOT exceed 12 digits.
 *
 * @param[in] str String matching the regex \d+(\.\d+)?, cannot be NULL
 * @param[out] value_out Where to store result on success, cannot be NULL
 * @return 0 on success, -84 if the integer part is too long or other error code otherwise (see StringToLong source code)
 */
int StringDecimalToLong(const char *str, long *value_out)
{
    assert(str != NULL);
    assert(value_out != NULL);

    size_t len_of_int_part = strcspn(str, ".");

    if (len_of_int_part > PRINTSIZE(INT_MAX))
    {
        return -84;  // Integer part too large to allocate buffer
    }

    if (len_of_int_part == 0 || str[len_of_int_part] == '\0')
    {
        return StringToLong(str, value_out);
    }

    char int_part[len_of_int_part + 1];
    strncpy(int_part, str, len_of_int_part);
    int_part[len_of_int_part] = '\0';

    return StringToLong(int_part, value_out);
}

/**
 * @brief Log StringToLong & StringToUlong & StringDecimalToLong conversion error
 *        with an identifier for debugging.
 */
void LogStringToLongError(const char *str_attempted, const char *id, int error_code)
{
    assert(error_code != 0);
    const char *error_str = "Unknown";
    switch (error_code)
    {
        case ERANGE:
            error_str = "Overflow";
            break;
        case -81:
            error_str = "No digits";
            break;
        case -82:
            error_str = "No endpointer";
            break;
        case -83:
            error_str = "Not terminated";
            break;
        case -84:
            error_str = "Integer part too large";
            break;
    }
    Log(LOG_LEVEL_ERR, "Conversion error (%d - %s) on '%s' (%s)", error_code, error_str, str_attempted, id);
}

/**
 * @brief Converts a string of numerals in base 10 to a long integer,
 *        uses a default value if errors occur.
 *
 * @see StringToLong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[in] default_return Value to return on error
 * @return Result of conversion (or default_return in case of error)
 */
long StringToLongDefaultOnError(const char *str, long default_return)
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
 * @brief Converts a string of numerals in base 10 to a long integer, exits on error.
 *
 * Only use this function in contexts/components where it is acceptable
 * to immediately exit when something goes wrong.
 *
 * @warning This function can exit the process based on string contents
 * @see StringToLong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @return Result of conversion
 */
long StringToLongExitOnError(const char *str)
{
    assert(str != NULL);
    long result;
    int return_code = StringToLong(str, &result);
    if (return_code != 0)
    {
        LogStringToLongError(str, "StringToLongExitOnError", return_code);
        DoCleanupAndExit(EXIT_FAILURE);
    }
    return result;
}

/**
 * @brief Converts a string of numerals in base 10 to a unsigned long 
 *        integer.
 *
 * Result is stored in *value_out, return value should be checked.
 * On error *value_out is unmodified and an error code is returned.
 * Leading spaces in input string are skipped.
 * String numeral must be terminated by NULL byte or space (isspace).
 *
 * @see StringToUlongExitOnError()
 * @see StringToUlongDefaultOnError()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[out] value_out Where to store result on success, cannot be NULL
 * @return 0 on success, error code otherwise (see source code)
 */
int StringToUlong(const char *str, unsigned long *value_out)
{
    nt_static_assert(ERANGE != 0);
    assert(str != NULL);
    assert(value_out != NULL);

    char *endptr = NULL;

    errno = 0;
    const unsigned long val = strtoul(str, &endptr, 10);

    if ((errno == ERANGE && val == ULONG_MAX))
    {
        return ERANGE; // Overflow
    }

    // Negative numbers should cause underflow
    if (val > 0) // "-0" is ok
    {
        const char *ch = str;
        while (ch < endptr && isspace(*ch)) ch++;
        if (*ch == '-')
        {
            return ERANGE; // Underflow
        }
    }

    if (endptr == str)
    {
        return -81; // No digits found
    }

    if (endptr == NULL)
    {
        return -82; // endpointer not set by strtol
    }

    if (*endptr != '\0' && !isspace(*endptr))
    {
        return -83; // string not properly terminated
    }

    if (errno != 0)
    {
        return errno; // Unknown error
    }

    *value_out = val;
    return 0;
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
unsigned long StringToUlongDefaultOnError(const char *str, unsigned long default_return)
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
 * @brief Converts a string of numerals in base 10 to a unsigned long 
 *        integer, exits on error.
 *
 * Only use this function in contexts/components where it is acceptable
 * to immediately exit when something goes wrong.
 *
 * @warning This function can exit the process based on string contents
 * @see StringToUlong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @return Result of conversion
 */
unsigned long StringToUlongExitOnError(const char *str)
{
    assert(str != NULL);
    unsigned long result;
    int return_code = StringToUlong(str, &result);
    if (return_code != 0)
    {
        LogStringToLongError(str, "StringToUlongExitOnError", return_code);
        DoCleanupAndExit(EXIT_FAILURE);
    }
    return result;
}

/**
 * @brief Converts a string of numerals in base 10 to 64-bit signed int
 *
 * Result is stored in *value_out, return value should be checked.
 * On error *value_out is unmodified and false is returned.
 *
 * @see StringToLong()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @param[out] value_out Where to store result on success, cannot be NULL
 * @return 0 on success, error code otherwise (see source code)
 */
int StringToInt64(const char *str, int64_t *value_out)
{
    nt_static_assert(sizeof(int64_t) == sizeof(intmax_t));
    nt_static_assert(ERANGE != 0);
    assert(str != NULL);
    assert(value_out != NULL);

    char *endptr = NULL;
    int64_t val;

    errno = 0;
    val = strtoimax(str, &endptr, 10);

    if ((errno == ERANGE && (val == INTMAX_MAX || val == INTMAX_MIN)))
    {
        return ERANGE; // Overflow or underflow
    }

    if (endptr == str)
    {
        return -81; // No digits found
    }

    if (endptr == NULL)
    {
        return -82; // endpointer not set by strtol
    }

    if (*endptr != '\0' && !isspace(*endptr))
    {
        return -83; // string not properly terminated
    }

    if (errno != 0)
    {
        return errno; // Unknown error
    }

    *value_out = val;
    return 0;
}

/**
 * @brief Convert a string to int64_t, exits if parsing fails
 *
 * Only use this function in contexts/components where it is acceptable
 * to immediately exit when something goes wrong.
 *
 * @warning This function can exit the process based on string contents
 * @see StringToInt64()
 * @see StringToLongExitOnError()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @return Result of conversion
 */
int64_t StringToInt64ExitOnError(const char *str)
{
    assert(str != NULL);

    int64_t result;
    const int error_code = StringToInt64(str, &result);
    if (error_code != 0)
    {
        LogStringToLongError(str, "StringToInt64ExitOnError", error_code);
        DoCleanupAndExit(EXIT_FAILURE);
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
int64_t StringToInt64DefaultOnError(const char *str, int64_t default_return)
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

/**
 * @brief Convert a string of numerals to a long integer (deprecated).
 *
 * @warning This function is deprecated, do not use it
 * @warning This function can exit the process based on string contents
 * @see StringToLongExitOnError()
 * @param[in] str String with numerals to convert, cannot be NULL
 * @return Result of conversion
 */
long StringToLongUnsafe(const char *str)
{
    assert(str);

    char *end;
    long result = strtol(str, &end, 10);

    // This is bad:
    assert(!*end && "Failed to convert string to long");

    return result;
}

char *StringFromLong(long number)
{
    char *str = xcalloc(32, sizeof(char));
    snprintf(str, 32, "%ld", number);
    return str;
}

/*********************************************************************/

double StringToDouble(const char *str)
{
    assert(str);

    char *end;
    double result = strtod(str, &end);

    assert(!*end && "Failed to convert string to double");

    return result;
}

char *StringFromDouble(double number)
{
    return StringFormat("%.2f", number);
}

/*********************************************************************/

char *NULLStringToEmpty(char *str)
{
    if(!str)
    {
        return "";
    }

    return str;
}

/**
 * @NOTE this function always '\0'-terminates the destination string #dst.
 * @return length of written string #dst.
 */
size_t StringBytesToHex(char *dst, size_t dst_size,
                        const unsigned char *src_bytes, size_t src_len)
{
    static const char *const hex_chars = "0123456789abcdef";

    size_t i = 0;
    while ((i < src_len) &&
           (i*2 + 2 < dst_size))               /* room for 2 more hex chars */
    {
        dst[2*i]     = hex_chars[(src_bytes[i] >> 4) & 0xf];
        dst[2*i + 1] = hex_chars[src_bytes[i] & 0xf];
        i++;
    }

    assert(2*i < dst_size);
    dst[2*i] = '\0';

    return 2*i;
}

bool IsStrIn(const char *str, const char *const strs[])
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

bool IsStrCaseIn(const char *str, const char *const strs[])
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

int CountChar(const char *string, char sep)
{
    int count = 0;

    if (string == NULL)
    {
        return 0;
    }

    if (string && (strlen(string) == 0))
    {
        return 0;
    }

    for (const char *sp = string; *sp != '\0'; sp++)
    {
        if ((*sp == '\\') && (*(sp + 1) == sep))
        {
            ++sp;
        }
        else if (*sp == sep)
        {
            count++;
        }
    }

    return count;
}

void ReplaceChar(const char *in, char *out, int outSz, char from, char to)
/* Replaces all occurrences of 'from' to 'to' in preallocated
 * string 'out'. */
{
    int len;
    int i;

    memset(out, 0, outSz);
    len = strlen(in);

    for (i = 0; (i < len) && (i < outSz - 1); i++)
    {
        if (in[i] == from)
        {
            out[i] = to;
        }
        else
        {
            out[i] = in[i];
        }
    }
}

/**
 * Replace all occurrences of #find with #replace.
 *
 * @return the length of #buf or -1 in case of overflow, or 0 if no replace
 *         took place.
 */
ssize_t StringReplace(char *buf, size_t buf_size,
                      const char *find, const char *replace)
{
    return StringReplaceN(buf, buf_size, find, replace, buf_size);
}


/**
 * Replace the first at most #n occurrences of #find in #buf with #replace.
 *
 * @return the length of #buf or -1 in case of overflow, or 0 if no replace
 *         took place.
 */
ssize_t StringReplaceN(char *buf, size_t buf_size,
                       const char *find, const char *replace, size_t n)
{
    assert(find[0] != '\0');

    if (n == 0)
    {
        return 0;
    }

    char *p = strstr(buf, find);
    if (p == NULL)
    {
        return 0;
    }

    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    size_t buf_len = strlen(buf);
    size_t buf_idx = 0;
    char tmp[buf_size];
    ssize_t tmp_len = 0;

    /* Do all replacements we find. */
    do
    {
        size_t buf_newidx = p - buf;
        size_t prefix_len = buf_newidx - buf_idx;

        if (tmp_len + prefix_len + replace_len >= buf_size)
        {
            return -1;
        }

        memcpy(&tmp[tmp_len], &buf[buf_idx], prefix_len);
        tmp_len += prefix_len;

        memcpy(&tmp[tmp_len], replace, replace_len);
        tmp_len += replace_len;

        buf_idx = buf_newidx + find_len;
        p = strstr(&buf[buf_idx], find);
        n--;
    } while ((p != NULL) && (n > 0));

    /* Copy leftover plus terminating '\0'. */
    size_t leftover_len = buf_len - buf_idx;
    if (tmp_len + leftover_len >= buf_size)
    {
        return -1;
    }
    memcpy(&tmp[tmp_len], &buf[buf_idx], leftover_len + 1);
    tmp_len += leftover_len;

    /* And finally copy to source, we are supposed to modify it in place. */
    memcpy(buf, tmp, tmp_len + 1);

    return tmp_len;
}

void ReplaceTrailingChar(char *str, char from, char to)
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

static StringRef StringRefNull(void)
{
    return (StringRef) { .data = NULL, .len = 0 };
}

size_t StringCountTokens(const char *str, size_t len, const char *seps)
{
    size_t num_tokens = 0;
    bool in_token = false;

    for (size_t i = 0; i < len; i++)
    {
        if (strchr(seps, str[i]))
        {
            in_token = false;
        }
        else
        {
            if (!in_token)
            {
                num_tokens++;
            }
            in_token = true;
        }
    }

    return num_tokens;
}

static StringRef StringNextToken(const char *str, size_t len, const char *seps)
{
    size_t start = 0;
    bool found = false;
    for (size_t i = 0; i < len; i++)
    {
        if (strchr(seps, str[i]))
        {
            if (found)
            {
                assert(i > 0);
                return (StringRef) { .data = str + start, .len = i - start };
            }
        }
        else
        {
            if (!found)
            {
                found = true;
                start = i;
            }
        }
    }

    if (found)
    {
        return (StringRef) { .data = str + start, .len = len - start };
    }
    else
    {
        return StringRefNull();
    }
}

StringRef StringGetToken(const char *str, size_t len, size_t index, const char *seps)
{
    StringRef ref = StringNextToken(str, len, seps);
    for (size_t i = 0; i < index; i++)
    {
        if (!ref.data)
        {
            return ref;
        }

        len = len - (ref.data - str + ref.len);
        str = ref.data + ref.len;

        ref = StringNextToken(str, len, seps);
    }

    return ref;
}

char **String2StringArray(const char *str, char separator)
/**
 * Parse CSVs into char **.
 * MEMORY NOTE: Caller must free return value with FreeStringArray().
 **/
{
    int i = 0, len;

    if (str == NULL)
    {
        return NULL;
    }

    for (const char *sp = str; *sp != '\0'; sp++)
    {
        if (*sp == separator)
        {
            i++;
        }
    }

    char **arr = (char **) xcalloc(i + 2, sizeof(char *));

    const char *sp = str;
    i = 0;

    while (sp)
    {
        const char *esp = strchr(sp, separator);

        if (esp)
        {
            len = esp - sp;
            esp++;
        }
        else
        {
            len = strlen(sp);
        }

        arr[i] = xcalloc(len + 1, sizeof(char));
        memcpy(arr[i], sp, len);

        sp = esp;
        i++;
    }

    return arr;
}

void FreeStringArray(char **strs)
{
    int i;

    if (strs == NULL)
    {
        return;
    }

    for (i = 0; strs[i] != NULL; i++)
    {
        free(strs[i]);
        strs[i] = NULL;
    }

    free(strs);
}


char *EscapeCharCopy(const char *str, char to_escape, char escape_with)
/*
 * Escapes the 'to_escape'-chars found in str, by prefixing them with 'escape_with'.
 * Returns newly allocated string.
 */
{
    assert(str);

    size_t in_size = strlen(str);

    if(in_size > (SIZE_MAX / 2) - 1)
    {
        ProgrammingError("Buffer passed to EscapeCharCopy() too large (in_size=%zd)", in_size);
    }

    size_t out_size = in_size + CountChar(str, to_escape) + 1;

    char *out = xcalloc(1, out_size);

    const char *in_pos = str;
    char *out_pos = out;

    for(; *in_pos != '\0'; in_pos++, out_pos++)
    {
        if(*in_pos == to_escape)
        {
            *out_pos = escape_with;
            out_pos++;
        }
        *out_pos = *in_pos;
    }

    return out;
}

char *ScanPastChars(char *scanpast, char *input)
{
    char *pos = input;

    while ((*pos != '\0') && (strchr(scanpast, *pos)))
    {
        pos++;
    }

    return pos;
}

int StripTrailingNewline(char *str, size_t max_length)
{
    if (str)
    {
        size_t i = strnlen(str, max_length + 1);
        if (i > max_length) /* See off-by-one comment below */
        {
            return -1;
        }

        while (i > 0 && str[i - 1] == '\n')
        {
            i--;
        }
        assert(str[i] == '\0' || str[i] == '\n');
        str[i] = '\0';
    }
    return 0;
}

/**
 * Removes trailing whitespace from a string.
 *
 * @WARNING Off-by-one quirk in max_length.
 *
 * Both StripTrailngNewline() and Chop() have long allowed callers to
 * pass (effectively) the strlen(str) rather than the size of memory
 * (which is at least strlen() + 1) in the buffer.  The present
 * incarnation thus reads max_length as max-strlen(), not as size of
 * the buffer.  It may be sensible to review all callers and change so
 * that max_length is the buffer size instead.
 *
 * TODO change Chop() to accept str_len parameter. It goes without saying that
 * the callers should call it as:    Chop(s, strlen(s));
 */

int Chop(char *str, size_t max_length)
{
    if (str)
    {
        size_t i = strnlen(str, max_length + 1);
        if (i > max_length) /* See off-by-one comment above */
        {
            /* Given that many callers don't even check Chop's return value,
             * we should NULL-terminate the string here. TODO. */
            return -1;
        }

        while (i > 0 && isspace((unsigned char)str[i-1]))
        {
            i--;
        }
        assert(str[i] == '\0' || isspace((unsigned char)str[i]));
        str[i] = '\0';
    }
    return 0;
}

char *TrimWhitespace(char *s)
{
    assert(s);

    // Leading whitespace:
    while (isspace(s[0]))
    {
        ++s;
    }

    // Empty string (only whitespace):
    if (s[0] == '\0')
    {
        return s;
    }

    // Trailing whitespace:
    char *end = s + strlen(s) - 1; // Last byte before '\0'
    while ( isspace(end[0]) )
    {
        --end;
    }
    end[1] = '\0';   // Null terminate string after last non space char

    return s;
}

/**
 * @brief Remove the CRLF at the end of a CSV line, if it exists
 *
 * For use in networking / reporting protocols. For old clients which send a
 * CRLF it will remove exactly 1 occurence of CRLF. For new clients which
 * don't send CRLF, it will remove nothing. Thus, it should be compatible
 * with both. Does not trim any other whitespace, because we don't expect any
 * other whitespace to be there, and if it did, this could mask important
 * errors.
 *
 * @see CsvWriterNewRecord()
 * @param data NUL-terminated string with one line of CSV data, cannot be NULL
 * @return strlen(data), after trimming
 */
size_t TrimCSVLineCRLF(char *const data)
{
    assert(data != NULL);

    size_t length = strlen(data);
    assert(data[length] == '\0');

    if (length < 2)
    {
        return length;
    }

    if (data[length - 2] == '\r' && data[length - 1] == '\n')
    {
        data[length - 2] = '\0';
        data[length - 1] = '\0';
        length -= 2;
    }

    assert(length == strlen(data));

    return length;
}

/**
 * @brief Remove the CRLF at the end of a non-empty CSV line, if it exists
 *
 * Based on `TrimCSVLineCRLF()`, adds a lot of assertions for bad cases we
 * want to detect in testing.
 *
 * @see TrimCSVLineCRLF()
 * @param data NUL-terminated string with one line of CSV data, cannot be NULL
 * @return strlen(data), after trimming
 */
size_t TrimCSVLineCRLFStrict(char *const data)
{
    assert(data != NULL);
    assert(strlen(data) > 0);
    assert(strlen(data) < 4096);
    assert(!isspace(data[0]));

    const size_t length = TrimCSVLineCRLF(data);
    assert(length == strlen(data));
    assert(length > 0);
    assert(!isspace(data[length - 1]));
    assert(!isspace(data[0]));

    return length;
}

void StringCloseHole(char *s, const size_t start, const size_t end)
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

bool StringEndsWithCase(const char *str, const char *suffix, const bool case_fold)
{
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len)
    {
        return false;
    }

    for (size_t i = 0; i < suffix_len; i++)
    {
        char a = str[str_len - i - 1];
        char b = suffix[suffix_len - i - 1];
        if (case_fold)
        {
            a = ToLower(a);
            b = ToLower(b);
        }

        if (a != b)
        {
            return false;
        }
    }

    return true;
}

bool StringEndsWith(const char *str, const char *suffix)
{
    return StringEndsWithCase(str, suffix, false);
}

bool StringStartsWith(const char *str, const char *prefix)
{
    int str_len = strlen(str);
    int prefix_len = strlen(prefix);

    if (prefix_len > str_len)
    {
        return false;
    }

    for (int i = 0; i < prefix_len; i++)
    {
        if (str[i] != prefix[i])
        {
            return false;
        }
    }
    return true;
}


/**
 * Returns pointer to the first byte in #buf that is not #c. Returns NULL if
 * all of #buf contains only bytes of value #c.
 *
 * @NOTE this functions complements memchr() from POSIX.
 *
 * @TODO move to libcompat, it appears to be available in some systems.
 */
void *memcchr(const void *buf, int c, size_t buf_size)
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


/*
 * @brief extract info from input string given two types of constraints:
 *        - length of the extracted string is bounded
 *        - extracted string should stop at first element of an exclude list
 *
 * @param[in] isp     : the string to scan
 * @param[in] limit   : size limit on the output string (including '\0')
 * @param[in] exclude : characters to be excluded from output buffer
 * @param[out] obuf   : the output buffer
 * @retval    true if string was capped, false if not
 */
bool StringNotMatchingSetCapped(const char *isp, size_t limit,
                      const char *exclude, char *obuf)
{
    size_t l = strcspn(isp, exclude);

    if (l < limit-1)
    {
        memcpy(obuf, isp, l);
        obuf[l]='\0';
        return false;
    }
    else
    {
        memcpy(obuf, isp, limit-1);
        obuf[limit-1]='\0';
        return true;
    }
}

bool StringAppend(char *dst, const char *src, size_t n)
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

/**
 *  Canonify #src into #dst.
 *
 *  @WARNING you must make sure sizeof(dst) is adequate!
 *
 *  @return always #dst.
 */
char *StringCanonify(char *dst, const char *src)
{
    while (*src != '\0')
    {
        if (isalnum((unsigned char) *src))
        {
            *dst = *src;
        }
        else
        {
            *dst = '_';
        }

        src++;
        dst++;
    }
    *dst = '\0';

    return dst;
}

/**
 *  Append #sep if not already there, and then append #leaf.
 */
bool PathAppend(char *path, size_t path_size, const char *leaf, char sep)
{
    size_t path_len = strlen(path);
    size_t leaf_len = strlen(leaf);

    if (path_len > 0 && path[path_len - 1] == sep)
    {
        /* Path already has separator. */
        path_len--;
    }

    if (path_len + 1 + leaf_len >= path_size)
    {
        return false;                                           /* overflow */
    }

    path[path_len] = sep;
    memcpy(&path[path_len + 1], leaf, leaf_len + 1);

    return true;
}

/**
 * Append #src string to #dst, useful for static allocated buffers.
 *
 * @param #dst_len if not NULL then it must contain the lenth of #dst and must
 *                 return the total needed length of the result in #dst, even
 *                 when truncation occurs.
 * @param #src_len if not 0, then this many bytes
 *                 from #src shall be concatenated to #dst.
 *
 * @NOTE if truncation happened, then the value returned in #dst_len shall be
 *       greater than or equal to #dst_size (indicating the real size needed
 *       for successful concatenation), and the real length of the truncated
 *       string in #dst shall be #dst_size - 1.
 *
 * @NOTE it is legal for #dst_len to be >= dst_size already when the function
 *       is called. In that case nothing will be appended, only #dst_len will
 *       be updated with the extra size needed.
 */
void StrCat(char *dst, size_t dst_size, size_t *dst_len,
            const char *src, size_t src_len)
{
    size_t dlen = (dst_len != NULL) ? *dst_len : strlen(dst);
    size_t slen = (src_len != 0)    ?  src_len : strlen(src);

    size_t needed_len = dlen + slen;

    if (dlen + 1 >= dst_size)              /* dst already full or overflown */
    {
        /* Append nothing, only update *dst_len. */
    }
    else if (needed_len < dst_size)                             /* it fits */
    {
        memcpy(&dst[dlen], src, slen);
        dst[needed_len] = '\0';
    }
    else                                                      /* truncation */
    {
        assert(dlen + slen >= dst_size);
        memcpy(&dst[dlen], src, dst_size - dlen - 1);
        dst[dst_size - 1] = '\0';
    }

    if (dst_len != NULL)
    {
        *dst_len = needed_len;
    }
}

/**
 * Append #src to #dst, delimited with #sep if #dst was not empty.
 *
 * @param #dst_len In-out parameter. If not NULL it is taken into account as
 *                 the current length of #dst, and updated as such.
 *
 * @NOTE if after returning *dst_len>=dst_size, this indicates that *nothing
 *       was appended*, and the value is the size needed for success.
 *
 */
void StrCatDelim(char *dst, size_t dst_size, size_t *dst_len,
                 const char *src, char sep)
{
    size_t dlen = (dst_len != NULL) ? *dst_len : strlen(dst);
    size_t slen = strlen(src);

    size_t needed_len = dlen + slen;
    if (dlen > 0)
    {
        needed_len++;                        /* separator must be prepended */
    }

    if (dlen + 1 >= dst_size)              /* dst already full or overflown */
    {
        /* Append nothing, only update *dst_len. */
    }
    else if (needed_len < dst_size)                             /* it fits */
    {
        /* Prepend separator if not empty. */
        if (dlen > 0)
        {
            dst[dlen] = sep;;
            dlen++;
        }
        memcpy(&dst[dlen], src, slen);
        dst[needed_len] = '\0';
    }
    else                                                    /* does not fit */
    {
        /* Append nothing, only update *dst_len. */
    }

    if (dst_len != NULL)
    {
        *dst_len = needed_len;
    }
}

/*********************************************************************/

void CanonifyNameInPlace(char *s)
{
    for (; *s != '\0'; s++)
    {
        if (!isalnum((unsigned char) *s))
        {
            *s = '_';
        }
    }
}

bool StringMatchesOption(
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

ssize_t StringFind(
    const char *const str,
    const char *const sub,
    const size_t from,
    const size_t to)
{
    assert(str != NULL);
    assert(sub != NULL);

    const size_t strl = strlen(str);
    const size_t subl = strlen(sub);

    for (size_t i = from; i < MIN(strl, to); i++)
    {
        if (strncmp(str + i, sub, subl) == 0)
        {
            return i;
        }
    }
    return -1;
}
