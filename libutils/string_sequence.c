#include <platform.h>
#include <string_sequence.h>
#include <string_lib.h> // SafeStringLength()
#include <alloc.h>  // xstrdup()
#include <string.h> // strlen()
#include <writer.h> // StringWriter()
#include <file_lib.h> // safe_fopen()

//////////////////////////////////////////////////////////////////////////////
// SeqString - Sequence of strings (char *)
//////////////////////////////////////////////////////////////////////////////

static void SeqStringAddSplit(Seq *seq, const char *str, char delimiter)
{
    if (str) // TODO: remove this inconsistency, add assert(str)
    {
        const char *prev = str;
        const char *cur = str;

        while (*cur != '\0')
        {
            if (*cur == delimiter)
            {
                size_t len = cur - prev;
                if (len > 0)
                {
                    SeqAppend(seq, xstrndup(prev, len));
                }
                else
                {
                    SeqAppend(seq, xstrdup(""));
                }
                prev = cur + 1;
            }

            cur++;
        }

        if (cur > prev)
        {
            SeqAppend(seq, xstrndup(prev, cur - prev));
        }
    }
}

Seq *SeqStringFromString(const char *str, char delimiter)
{
    Seq *seq = SeqNew(10, &free);

    SeqStringAddSplit(seq, str, delimiter);

    return seq;
}

bool SeqStringContains(const Seq *const seq, const char *const str)
{
    assert(seq != NULL);
    assert(str != NULL);

    size_t length = SeqLength(seq);
    for (size_t i = 0; i < length; ++i)
    {
        if (StringEqual(str, SeqAt(seq, i)))
        {
            return true;
        }
    }
    return false;
}

int SeqStringLength(Seq *seq)
{
    assert(seq);

    int total_length = 0;
    size_t seq_length = SeqLength(seq);
    for (size_t i = 0; i < seq_length; i++)
    {
        total_length += SafeStringLength(SeqAt(seq, i));
    }

    return total_length;
}

// TODO: These static helper functions could be (re)moved
static bool HasNulByte(const char *str, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (str[i] == '\0')
        {
            return true;
        }
    }
    return false;
}

static long GetLengthPrefix(const char *data)
{
    if (HasNulByte(data, 10))
    {
        return -1;
    }

    if (!isdigit(data[0]))
    {
        return -1;
    }

    if (data[STR_LENGTH_PREFIX_LEN - 1] != ' ')
    {
        return -1;
    }

    // NOTE: This uses long because HPUX sscanf doesn't support %zu
    long length;
    int ret = sscanf(data, "%ld", &length);
    if (ret != 1 || length < 0)
    {
        // Incorrect number of items matched, or
        // negative length prefix(invalid)
        return -1;
    }

    return length;
}

static char *ValidDuplicate(const char *src, size_t n)
{
    assert(src != NULL);
    char *dst = xcalloc(n + 1, sizeof(char));

    size_t len = StringCopy(src, dst, n + 1);
    // If string was too long, len >= n+1, this is OKAY, there's more data.
    if (len < n) // string was too short
    {
        free(dst);
        return NULL;
    }

    return dst;
}

bool WriteLenPrefixedString(Writer *w, const char *string)
{
    const size_t str_length = strlen(string);
    const size_t bytes_written = WriterWriteF(
        w, "%-" TO_STRING(STR_LENGTH_PREFIX_LEN) "zu%s\n", str_length, string);
    // TODO: Make WriterWriteF actually be able to propagate errors
    //       (return negative number on short writes).
    if (bytes_written == 0)
    {
        return false;
    }
    return true;
}

bool SeqStringWrite(Seq *seq, Writer *w)
{
    const size_t length = SeqLength(seq);
    for (size_t i = 0; i < length; ++i)
    {
        const char *const s = SeqAt(seq, i);
        bool success = WriteLenPrefixedString(w, s);
        if (!success)
        {
            return false;
        }
    }
    return true;
}

char *SeqStringSerialize(Seq *seq)
{
    Writer *w = StringWriter();
    SeqStringWrite(seq, w);
    return StringWriterClose(w);
}

bool SeqStringWriteFileStream(Seq *seq, FILE *file)
{
    Writer *w = FileWriter(file);
    assert(w != NULL);

    bool success = SeqStringWrite(seq, w);
    FileWriterDetach(w);
    return success;
}

bool SeqStringWriteFile(Seq *seq, const char *file)
{
    FILE *f = safe_fopen(file, "w");
    if (f == NULL)
    {
        return false;
    }

    const bool write_success = SeqStringWriteFileStream(seq, f);
    const bool close_success = (fclose(f) == 0);
    return (write_success && close_success);
}

Seq *SeqStringDeserialize(const char *const serialized)
{
    assert(serialized != NULL);
    assert(STR_LENGTH_PREFIX_LEN > 0);

    Seq *seq = SeqNew(128, free);

    const char *src = serialized;
    while (src[0] != '\0')
    {
        // Read length prefix first
        long length = GetLengthPrefix(src);

        // Advance the src pointer
        src += STR_LENGTH_PREFIX_LEN;

        char *new_str = NULL;

        // Do validation and duplication in one pass
        // ValidDuplicate checks for terminating byte up to src[length-1]
        if (length < 0 || src[-1] != ' ' ||
            NULL == (new_str = ValidDuplicate(src, length)) ||
            src[length] != '\n')
        {
            free(new_str);
            SeqDestroy(seq);
            return NULL;
        }

        SeqAppend(seq, new_str);

        // Advance src pointer
        src += length + 1; // +1 for the added newline
    }

    return seq;
}

int ReadLenPrefixedString(int fd, char **string)
{
    char prefix[STR_LENGTH_PREFIX_LEN];
    ssize_t bytes_read = FullRead(fd, prefix, STR_LENGTH_PREFIX_LEN);
    if (bytes_read == 0)
    {
        // EOF
        return 0;
    }
    if (bytes_read < 0)
    {
        // Error
        return -1;
    }
    assert(prefix[STR_LENGTH_PREFIX_LEN - 1] == ' '); // NOTE: Not NUL-terminated
    const long length = GetLengthPrefix(prefix);

    // Read data, followed by a '\n' which we replace with '\0':
    const long size = length + 1;
    char *const data = xmalloc(size);
    bytes_read = FullRead(fd, data, size);
    if (bytes_read != size || data[length] != '\n')
    {
        // Short read, or error, or missing newline
        free(data);
        return -1;
    }
    data[length] = '\0'; // Replace newline with NUL-terminator
    *string = data;

    return 1;
}

Seq *SeqStringReadFile(const char *file)
{
    const int fd = safe_open(file, O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    Seq *seq = SeqNew(500, &free);

    while (true)
    {
        char *data;
        int ret = ReadLenPrefixedString(fd, &data);
        if (ret < 0)
        {
            /* error */
            SeqDestroy(seq);
            return NULL;
        }
        else if (ret == 0)
        {
            /* done (EOF) */
            return seq;
        }
        else
        {
            SeqAppend(seq, data);
        }
    }
}
