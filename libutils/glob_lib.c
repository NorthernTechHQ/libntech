#include <platform.h>
#include <glob_lib.h>
#include <alloc.h>
#include <file_lib.h>
#include <string_lib.h>
#include <logging.h>
#include <sequence.h>
#include <string_sequence.h>
#include <path.h>

/**
 * If not compiled with PCRE2, we fallback to glob(3) implementation. Why not
 * use glob(3) and fallback on our implementation? Because commonly used
 * features like brace expansion is not defined by POSIX, and would require
 * custom implementation anyways. We don't want subtle differences in features
 * for different platforms.
 */
#ifdef WITH_PCRE2
#include <regex.h>
#else // WITH_PCRE2
#include <glob.h>
#endif // WITH_PCRE2

/**
 * On Windows we accept both forward- & backslash, otherwise only forward slash
 * is accepted.
 */
#ifdef _WIN32
#define PATH_DELIMITERS "\\/"
#else // _WIN32
#define PATH_DELIMITERS "/"
#endif // _WIN32

/**
 * Returns a set of brace-expanded patterns.
 */
static void ExpandBraces(const char *pattern, StringSet *expanded)
    FUNC_UNUSED; // Unused if compiled without PCRE2, but tested in unit tests

static void ExpandBraces(const char *const pattern, StringSet *const expanded)
{
    assert(pattern != NULL);
    assert(expanded != NULL);

    char *const start = SafeStringDuplicate(pattern);

    /* First we will split the pattern into three parts based on the curly
     * brace delimiters. We look for the shortest possible match. E.g., the
     * string "foo{{bar,baz}qux,}" becomes "foo{", "bar,baz", "qux,}".
     */
    char *left = NULL, *right = NULL;
    for (char *ch = start; *ch != '\0'; ch++)
    {
        if (*ch == '{')
        {
            left = ch;
        }
        else if (left != NULL && *ch == '}')
        {
            right = ch;
            break;
        }
    }

    /* Next we check if base case is reached. I.e., if there is no curly brace
     * pair to expand, then add the pattern itself to the result.
     */
    if (right == NULL)
    {
        StringSetAdd(expanded, start);
        return;
    }
    *left = '\0';
    *right = '\0';

    /* Next we split the middle part (between the braces) on the comma
     * delimiter. E.g., the string "bar,baz" becomes "bar", "baz". If there is
     * no comma in the middle part, then a sequence of length 1 is returned,
     * containing the entire middle part, and we still continue the recursion,
     * because the result is added as a part of the base case in the next
     * recursive layer.
     */
    char *middle = left + 1, *end = right + 1;
    Seq *const split = StringSplit(middle, ",");

    /* Lastly we combine the three parts for each split element, and
     * recursively expand subsequent curly braces.
     */
    const size_t len = SeqLength(split);
    for (size_t i = 0; i < len; i++)
    {
        middle = SeqAt(split, i);
        char *const next = StringConcatenate(3, start, middle, end);
        ExpandBraces(next, expanded);
        free(next);
    }

    free(start);
    SeqDestroy(split);
}

/**
 * Case normalized the path on Windows, so that we can use normal string
 * comparison in order to compare files. E.g., 'C:/Program Files' becomes
 * 'c:\program files'.
 */
static char *NormalizePath(const char *path)
    FUNC_UNUSED; // Unused if compiled without PCRE2, but tested in unit tests

static char *NormalizePath(const char *path)
{
    assert(path != NULL);
    char *const current = xstrdup(path);

#ifdef _WIN32
    ToLowerStrInplace(current);
#endif // _WIN32

    return current;
}

#ifdef WITH_PCRE2

/**
 * Translate the square bracket part of a shell expression (glob) into a
 * regular expression.
 */
static size_t TranslateBracket(
    const char *const pattern, const size_t n, size_t left, Buffer *const buf)
{
    size_t right = left;
    if (right < n && pattern[right] == '!')
    {
        right += 1;
    }
    if (right < n && pattern[right] == ']')
    {
        right += 1;
    }
    while (right < n && pattern[right] != ']')
    {
        right += 1;
    }
    if (right >= n)
    {
        /* There was an opening bracket, but no closing one. Treat it as part
         * of the filename by escaping it.
         */
        BufferAppendString(buf, "\\[");
        return left;
    }

    char *stuff = SafeStringNDuplicate(pattern + left, right - left);
    if (StringContains(stuff, "--"))
    {
        // Escape backslashes and hyphens for set difference (--).
        // Hyphens that create ranges shouldn't be escaped.
        Seq *const chunks = SeqNew(1, free);
        ssize_t middle = (pattern[left] == '!') ? left + 2 : left + 1;
        while (true)
        {
            middle = StringFind(pattern, "-", middle, right);
            if (middle < 0)
            {
                break;
            }

            char *chunk = SafeStringNDuplicate(pattern + left, middle - left);
            char *tmp = SearchAndReplace(chunk, "\\", "\\\\");
            FREE_AND_NULL(chunk);
            chunk = SearchAndReplace(tmp, "-", "\\-");
            free(tmp);
            SeqAppend(chunks, chunk);

            left = middle + 1;
            middle = middle + 3;
        }
        char *chunk = SafeStringNDuplicate(pattern + left, right - left);
        char *tmp = SearchAndReplace(chunk, "\\", "\\\\");
        FREE_AND_NULL(chunk);
        chunk = SearchAndReplace(tmp, "-", "\\-");
        FREE_AND_NULL(tmp);
        SeqAppend(chunks, chunk);

        tmp = StringJoin(chunks, "-");
        FREE_AND_NULL(stuff);
        stuff = tmp;
        SeqDestroy(chunks);
    }
    else
    {
        char *const tmp = SearchAndReplace(stuff, "\\", "\\\\");
        FREE_AND_NULL(stuff);
        stuff = tmp;
    }

    left = right + 1;
    if (stuff[0] == '!')
    {
        char *const tmp = StringConcatenate(2, "^", stuff + 1);
        FREE_AND_NULL(stuff);
        stuff = tmp;
    }
    else if (stuff[0] == '^' || stuff[0] == '[')
    {
        char *const tmp = StringConcatenate(2, "\\", stuff);
        FREE_AND_NULL(stuff);
        stuff = tmp;
    }

    BufferAppendF(buf, "[%s]", stuff);
    FREE_AND_NULL(stuff);

    return left;
}

/**
 * Translate a shell pattern to a regular expression.
 *
 * This is a C implementation of the translate function in the fnmatch module
 * from Python's standard library. See
 * https://github.com/python/cpython/blob/3.8/Lib/fnmatch.py
 */
static char *TranslateGlob(const char *const pattern)
{
    assert(pattern != NULL);

    static const char *const special_chars = "()[]{}?*+-|^$\\.&~# \t\n\r\v\f";
    size_t i = 0;
    const size_t n = strlen(pattern);
    Buffer *const buf = BufferNew();

    while (i < n)
    {
        char ch = pattern[i++];

        switch (ch)
        {
#ifndef _WIN32
        /* We don't allow escaping wildcards in Windows for two reasons. First,
         * we cannot escape with backslash when it is also a path separator.
         * Secondly, '*' is an illegal filename on Windows.
         */
        case '\\':
            BufferAppendChar(buf, ch);
            BufferAppendChar(buf, pattern[i++]);
            break;
#endif // _WIN32
        case '*':
            BufferAppendString(buf, ".*");
            break;

        case '?':
            BufferAppendString(buf, ".");
            break;

        case '[':
            i = TranslateBracket(pattern, n, i, buf);
            break;

        default:
            if (strchr(special_chars, ch) != NULL)
            {
                BufferAppendF(buf, "\\%c", ch);
            }
            else
            {
                BufferAppendChar(buf, ch);
            }
            break;
        }
    }

    char *const res = StringFormat("(?s:%s)\\Z", BufferData(buf));
    BufferDestroy(buf);
    return res;
}

bool GlobMatch(const char *const _pattern, const char *const _filename)
{
    assert(_pattern != NULL);
    assert(_filename != NULL);

    char *pattern = NormalizePath(_pattern);
    char *const filename = NormalizePath(_filename);

    StringSet *const expanded = StringSetNew();
    ExpandBraces(pattern, expanded);
    FREE_AND_NULL(pattern);

    bool any_matches = false;
    StringSetIterator iter = StringSetIteratorInit(expanded);
    while ((pattern = StringSetIteratorNext(&iter)) != NULL)
    {
        char *const regex = TranslateGlob(pattern);
        any_matches |= StringMatchFull(regex, filename);
        free(regex);
    }

    StringSetDestroy(expanded);
    free(filename);

    return any_matches;
}

/**
 * Structure used in conjunction with PathWalk() in order to pass arbitrary
 * data to callback function.
 */
typedef struct
{
    Seq *components;
    Seq *matches;
} GlobFindData;

/**
 * Used before each recursive branch in PathWalk() to duplicate arbitrary data.
 */
static void *GlobFindDataCopy(void *const data)
{
    assert(data != NULL);

    GlobFindData *const old = data;
    GlobFindData *const new = malloc(sizeof(GlobFindData));

    const size_t length = SeqLength(old->components);
    new->components = SeqNew(length, free);
    for (size_t i = 0; i < length; i++)
    {
        const char *const item = SeqAt(old->components, i);
        char *const copy = xstrdup(item);
        SeqAppend(new->components, copy);
    }

    // Note that we shallow copy matches.
    new->matches = old->matches;
    return new;
}

/**
 * Used after each recursive branch in PathWalk() to free duplicated arbitrary
 * data.
 */
static void GlobFindDataDestroy(void *const _data)
{
    assert(_data != NULL);

    GlobFindData *const data = _data;
    SeqDestroy(data->components);
    free(data);
}

/**
 * Callback function used in conjunction with PathWalk() in order to find glob
 * matches.
 */
static void PathWalkCallback(
    const char *const dirpath,
    Seq *const dirnames,
    const Seq *const filenames,
    void *const _data)
{
    assert(dirpath != NULL);
    assert(dirnames != NULL);
    assert(filenames != NULL);
    assert(_data != NULL);

    GlobFindData *const data = (GlobFindData *) _data;
    assert(data->components != NULL);
    assert(data->matches != NULL);

    const size_t n_components = SeqLength(data->components);
    if (n_components == 0)
    {
        /* We have matched each and every part of the glob pattern, thus we
         * have a full match. */
        char *const match = xstrdup(dirpath);
        SeqAppend(data->matches, match);
        Log(LOG_LEVEL_DEBUG,
            "Full match! Directory '%s' has matched all previous sub patterns",
            match);

        // Base case is reached and recursion ends here.
        SeqClear(dirnames);
        return;
    }

    // Pop the glob component at the head of sequence.
    char *const sub_pattern = SeqAt(data->components, 0);
    SeqSoftRemove(data->components, 0);

    /* Normally we would not iterate the '.' and '..' directory entries in
     * order to avoid infinite recursion. However, an exception is made when
     * they appear as a part of the glob pattern we are currently crunching. */
    if (StringEqual(sub_pattern, ".") || StringEqual(sub_pattern, ".."))
    {
        char *const entry = xstrdup(sub_pattern);
        SeqAppend(dirnames, entry);
    }

    const size_t n_dirnames = SeqLength(dirnames);
    for (size_t i = 0; i < n_dirnames; i++)
    {
        const char *const dirname = SeqAt(dirnames, i);
        if (!GlobMatch(sub_pattern, dirname))
        {
            /* Not a match! Make sure not to follow down this path. Setting the
             * directory name to NULL should do the trick. And it's more
             * efficient than removing it from the sequence. */
            SeqSet(dirnames, i, NULL);
        }
    }

    /* If number of remaining glob components to match is 1, it means that we
     * can start looking for non-directory matches. */
    if (n_components == 1)
    {
        const size_t n_filenames = SeqLength(filenames);
        for (size_t i = 0; i < n_filenames; i++)
        {
            const char *const filename = SeqAt(filenames, i);
            if (GlobMatch(sub_pattern, filename))
            {
                char *match = NULL;
                if (StringEqual(dirpath, "."))
                {
                    match = xstrdup(filename);
                }
                else
                {
                    match = Path_JoinAlloc(dirpath, filename);
                }
                SeqAppend(data->matches, match);
            }
        }
    }

    free(sub_pattern);
}

/**
 * Filter used in conjunction with SeqFilter() to remove empty strings from
 * sequence.
 */
static bool EmptyStringFilter(void *item)
{
    assert(item != NULL);
    const char *const str = (const char *) item;
    return StringEqual(str, "");
}

Seq *GlobFind(const char *pattern)
{
    assert(pattern != NULL);

    if (StringEqual(pattern, ""))
    {
        // Glob pattern is empty. Nothing to do.
        return SeqNew(0, free);
    }

    Seq *const matches = SeqNew(8 /* seems reasonable */, free);

    StringSet *expanded = StringSetNew();
    ExpandBraces(pattern, expanded);

    StringSetIterator iter = StringSetIteratorInit(expanded);
    while ((pattern = StringSetIteratorNext(&iter)) != NULL)
    {
        GlobFindData data = {
            .matches = matches,
            .components = StringSplit(pattern, PATH_DELIMITERS),
        };
        SeqFilter(data.components, EmptyStringFilter);

        if (IsAbsoluteFileName(pattern))
        {
            if (IsWindowsNetworkPath(pattern))
            {
                // Pop component at the head of sequence.
                const char *const hostname = SeqAt(data.components, 0);

                // Path like '\\hostname\...'.
                char *const path = StringFormat("\\\\%s", hostname);
                SeqRemoveRange(data.components, 0, 0);

                PathWalk(
                    path,
                    PathWalkCallback,
                    &data,
                    GlobFindDataCopy,
                    GlobFindDataDestroy);
                free(path);
            }
            else if (IsWindowsDiskPath(pattern))
            {
                // Path like 'C:\...'.
                // Pop component at the head of sequence.
                char *const path = SeqAt(data.components, 0);
                SeqSoftRemoveRange(data.components, 0, 0);

                PathWalk(
                    path,
                    PathWalkCallback,
                    &data,
                    GlobFindDataCopy,
                    GlobFindDataDestroy);
                free(path);
            }
            else
            {
                // Path like '/...'.
                PathWalk(
                    FILE_SEPARATOR_STR,
                    PathWalkCallback,
                    &data,
                    GlobFindDataCopy,
                    GlobFindDataDestroy);
            }
        }
        else
        {
            PathWalk(
                ".",
                PathWalkCallback,
                &data,
                GlobFindDataCopy,
                GlobFindDataDestroy);
        }

        SeqDestroy(data.components);
    }

    StringSetDestroy(expanded);
    SeqSort(matches, StrCmpWrapper, NULL);

    return matches;
}

#endif // WITH_PCRE2

StringSet *GlobFileList(const char *pattern)
{
    StringSet *set = StringSetNew();

    const char *replace[] = {
        "", "*", "*/*", "*/*/*", "*/*/*/*", "*/*/*/*/*", "*/*/*/*/*/*"};
    const bool double_asterisk = (strstr(pattern, "**") != NULL);
    const size_t replace_count =
        double_asterisk ? sizeof(replace) / sizeof(replace[0]) : 1;

    for (size_t i = 0; i < replace_count; i++)
    {
        char *expanded = double_asterisk
                             ? SearchAndReplace(pattern, "**", replace[i])
                             : SafeStringDuplicate(pattern);

#ifdef WITH_PCRE2
        Seq *const matches = GlobFind(expanded);
        const size_t num_matches = SeqLength(matches);
        for (size_t j = 0; j < num_matches; j++)
        {
            char *const match = SafeStringDuplicate(SeqAt(matches, j));
            StringSetAdd(set, match);
        }
        SeqDestroy(matches);
#else  // WITH_PCRE2
        glob_t globbuf;

        // If we don't have PCRE2, we fallback to the old way of doing things
        Log(LOG_LEVEL_WARNING,
            "Globs perform limited/buggy without PCRE2"
            " - consider recompiling libntech with PCRE2.");

        /* The glob(3) function keeps double slashes (e.g., "/etc//os-release")
         * in the result, if part of pattern. However, we want the result to be
         * similar to our PCRE2 solution. */
        char *const tmp = SearchAndReplace(expanded, "//", "/");
        free(expanded);
        expanded = tmp;

        if (glob(expanded, 0, NULL, &globbuf) == 0)
        {
            for (size_t j = 0; j < globbuf.gl_pathc; j++)
            {
                StringSetAdd(set, SafeStringDuplicate(globbuf.gl_pathv[j]));
            }
            globfree(&globbuf);
        }
#endif // WITH_PCRE2
        free(expanded);
    }

    return set;
}
