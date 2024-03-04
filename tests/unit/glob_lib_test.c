#include <test.h>
#include <glob_lib.c>

static void test_expand_braces(void)
{
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo", set);
        assert_true(StringSetContains(set, "foo"));
        assert_int_equal(StringSetSize(set), 1);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo{bar,baz}qux", set);
        assert_true(StringSetContains(set, "foobarqux"));
        assert_true(StringSetContains(set, "foobazqux"));
        assert_int_equal(StringSetSize(set), 2);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo{bar}baz", set);
        assert_true(StringSetContains(set, "foobarbaz"));
        assert_int_equal(StringSetSize(set), 1);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo{}bar", set);
        assert_true(StringSetContains(set, "foobar"));
        assert_int_equal(StringSetSize(set), 1);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo{,}bar", set);
        assert_true(StringSetContains(set, "foobar"));
        assert_int_equal(StringSetSize(set), 1);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("foo{{bar,baz}qux,}", set);
        assert_true(StringSetContains(set, "foo"));
        assert_true(StringSetContains(set, "foobarqux"));
        assert_true(StringSetContains(set, "foobazqux"));
        assert_int_equal(StringSetSize(set), 3);
        StringSetDestroy(set);
    }
    {
        StringSet *const set = StringSetNew();
        ExpandBraces("", set);
        assert_true(StringSetContains(set, ""));
        assert_int_equal(StringSetSize(set), 1);
        StringSetDestroy(set);
    }
}

static void test_normalize_path(void)
{
    char *const actual = NormalizePath("C:\\Program Files\\Cfengine\\bin\\");
#ifdef _WIN32
    assert_string_equal(actual, "c:\\program files\\cfengine\\bin\\");
#else  // _WIN32
    assert_string_equal(actual, "C:\\Program Files\\Cfengine\\bin\\");
#endif // _WIN32
    free(actual);
}

#ifdef WITH_PCRE2

static void test_translate_bracket(void)
{
    {
        const char *const pattern = "[a-z]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[a-z]");
        free(data);
    }
    {
        const char *const pattern = "[abc]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[abc]");
        free(data);
    }
    {
        const char *const pattern = "[!a-z]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[^a-z]");
        free(data);
    }
    {
        const char *const pattern = "[!abc]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[^abc]");
        free(data);
    }
    {
        const char *const pattern = "[a--c-f]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[a-\\-c-f]");
        free(data);
    }
    {
        const char *const pattern = "[[]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[\\[]");
        free(data);
    }
    {
        const char *const pattern = "[a-z+--A-Z]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[a-z+-\\-A-Z]");
        free(data);
    }
    {
        const char *const pattern = "[a-z--/A-Z]";
        Buffer *const buffer = BufferNew();
        TranslateBracket(pattern, strlen(pattern), 1, buffer);
        char *const data = BufferClose(buffer);
        assert_string_equal(data, "[a-z\\--/A-Z]");
        free(data);
    }
}

static void test_translate_glob(void)
{
    {
        char *const regex = TranslateGlob("*");
        assert_string_equal(regex, "(?s:.*)\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("?");
        assert_string_equal(regex, "(?s:.)\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("a?b*");
        assert_string_equal(regex, "(?s:a.b.*)\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("[abc]");
        assert_string_equal(regex, "(?s:[abc])\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("[]]");
        assert_string_equal(regex, "(?s:[]])\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("[!x]");
        assert_string_equal(regex, "(?s:[^x])\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("[x");
        assert_string_equal(regex, "(?s:\\[x)\\Z");
        free(regex);
    }
    {
        char *const regex = TranslateGlob("ba[rz]");
        assert_string_equal(regex, "(?s:ba[rz])\\Z");
        free(regex);
    }
}

static void test_glob_match(void)
{
    assert_true(GlobMatch("foo", "foo"));
    assert_false(GlobMatch("foo", "bar"));

    assert_true(GlobMatch("{foo,bar,}", "foo"));
    assert_true(GlobMatch("{foo,bar,}", "bar"));
    assert_false(GlobMatch("{foo,bar,}", "baz"));
    assert_true(GlobMatch("{foo,bar,}", ""));

    assert_true(GlobMatch("", ""));
    assert_false(GlobMatch("", "foo"));
    assert_false(GlobMatch("foo", ""));

    assert_true(GlobMatch("*", "foo"));
    assert_true(GlobMatch("*", ""));
    assert_true(GlobMatch("*", "*"));

    assert_false(GlobMatch("ba?", "foo"));
    assert_true(GlobMatch("ba?", "bar"));
    assert_true(GlobMatch("ba?", "baz"));

    assert_false(GlobMatch("ba[rz]", "foo"));
    assert_true(GlobMatch("ba[rz]", "bar"));
    assert_true(GlobMatch("ba[rz]", "baz"));
    assert_false(GlobMatch("ba[rz]", "bat"));
    assert_true(GlobMatch("ba[r-z]", "bat"));

    assert_true(GlobMatch("[a-z][a-z][a-z]", "foo"));
    assert_true(GlobMatch("[a-z][a-z][a-z]", "bar"));
    assert_true(GlobMatch("[a-z][a-z][a-z]", "baz"));
    assert_false(GlobMatch("[a-z][a-z][a-y]", "baz"));

    assert_true(GlobMatch("foo{{bar,baz}qux,}", "foo"));
    assert_true(GlobMatch("foo{{bar,baz}qux,}", "foobarqux"));
    assert_true(GlobMatch("foo{{bar,baz}qux,}", "foobazqux"));
    assert_false(GlobMatch("foo{{bar,baz}qux,}", "foobar"));
    assert_false(GlobMatch("foo{{bar,baz}qux,}", "foobaz"));

    assert_true(GlobMatch("[aaa]", "a"));
    assert_true(GlobMatch("[abc-f]", "a"));
    assert_true(GlobMatch("[abc-f]", "c"));
    assert_true(GlobMatch("[abc-f]", "d"));
    assert_true(GlobMatch("[ab-ef]", "d"));
    assert_true(GlobMatch("[ab-ef]", "f"));

    assert_true(GlobMatch("[!a-c][d-f]", "de"));
    assert_false(GlobMatch("[!a-c][d-f]", "cd"));
    assert_false(GlobMatch("[!a-c][d-f]", "dc"));
    assert_true(GlobMatch("[!abc]", "d"));
    assert_false(GlobMatch("[!abc]", "b"));

#ifdef _WIN32
    // Pattern and filename is case-normalized on Windows
    assert_true(GlobMatch(
        "C:\\Program Files\\Cfengine\\bin\\cf-agent.exe",
        "c:\\program files\\cfengine\\bin\\cf-agent.exe"));
    assert_true(GlobMatch(
        "c:\\program files\\cfengine\\bin\\cf-agent.exe",
        "C:\\Program Files\\Cfengine\\bin\\cf-agent.exe"));
#else  // _WIN32
    // Pattern and filename is not case-normalized on Unix
    assert_false(GlobMatch(
        "C:/Program Files/Cfengine/bin/cf-agent.exe",
        "c:/program files/cfengine/bin/cf-agent.exe"));
    assert_false(GlobMatch(
        "c:/program files/cfengine/bin/cf-agent.exe",
        "C:/Program Files/Cfengine/bin/cf-agent.exe"));
#endif // _WIN32

    assert_true(GlobMatch("[[]", "["));
    assert_true(GlobMatch("[a-z+--A-Z]", ","));
    assert_true(GlobMatch("[a-z--/A-Z]", "."));
}

static void test_glob_find(void)
{
    /* This test is not very thorough. However, test_glob_file_list()
     * indirectly tests this one. */
    {
        Seq *const matches = GlobFind("test.{h,c}");
        assert_int_equal(SeqLength(matches), 2);
        assert_string_equal(SeqAt(matches, 0), "test.c");
        assert_string_equal(SeqAt(matches, 1), "test.h");
        SeqDestroy(matches);
    }
    {
        Seq *const matches = GlobFind(".");
        assert_int_equal(SeqLength(matches), 1);
        assert_string_equal(SeqAt(matches, 0), ".");
        SeqDestroy(matches);
    }
}

#endif // WITH_PCRE2

static void test_glob_file_list(void)
{
    // Create temporary directory.

    char template[] = "test_glob_file_list_XXXXXX";
    const char *const test_dir = mkdtemp(template);
    assert_true(test_dir != NULL);

    // Create test subdirectories.

    static const char *const test_subdirs[] = {
        "foo",
        "foo" FILE_SEPARATOR_STR "bar",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR "baz",
        "qux",
    };
    const size_t num_test_subdirs =
        sizeof(test_subdirs) / sizeof(const char *);

    char path[PATH_MAX];
    for (size_t i = 0; i < num_test_subdirs; i++)
    {
        int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s",
            test_dir,
            test_subdirs[i]);
        assert_false(ret >= PATH_MAX || ret < 0);
        ret = mkdir(path, (mode_t) 0700);
        assert_int_equal(ret, 0);
    }

    // Create test files.

    static const char *const test_files[] = {
#ifdef WITH_PCRE2
        /* POSIX glob(3) does not support GLOB_PERIOD, meaning it will not find
         * this file, thus cause tests below to fail. */
        ".gitignore",
#endif // WITH_PCRE2
        "README.md",
        "foo" FILE_SEPARATOR_STR "README.md",
        "foo" FILE_SEPARATOR_STR "bank_statements.txt",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR "README.md",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR "passwords.txt",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR "secret.gpg",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR
        "baz" FILE_SEPARATOR_STR "README.md",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR
        "baz" FILE_SEPARATOR_STR "const is love",
        "foo" FILE_SEPARATOR_STR "bar" FILE_SEPARATOR_STR
        "baz" FILE_SEPARATOR_STR "const is life",
        "qux" FILE_SEPARATOR_STR "README.md",
        "qux" FILE_SEPARATOR_STR "source.c",
        "qux" FILE_SEPARATOR_STR "source.h",
        "qux" FILE_SEPARATOR_STR "source.o",
        "qux" FILE_SEPARATOR_STR "source.cpp",
        "qux" FILE_SEPARATOR_STR "source.hpp",
    };
    const size_t num_test_files = sizeof(test_files) / sizeof(const char *);

    for (size_t i = 0; i < num_test_files; i++)
    {
        const int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s",
            test_dir,
            test_files[i]);
        assert_true(ret < PATH_MAX && ret >= 0);

        const int fd = open(path, O_CREAT, (mode_t) 0600);
        assert_int_not_equal(fd, -1);
        close(fd);
    }

    /* Let's do some rigorous testing! The file system should look something
     * like this:
     *
     *    test_glob_file_list_XXXXXX/
     *    +-- .gitignore
     *    +-- README.md
     *    +-- foo/
     *    |   +-- README.md
     *    |   +-- bank_statements.txt
     *    |   +-- bar/
     *    |       +-- README.md
     *    |       +-- passwords.txt
     *    |       +-- secret.gpg
     *    |       +-- baz/
     *    |           +-- README.md
     *    |           +-- "const is love"
     *    |           +-- "const is life"
     *    +-- qux/
     *        +-- README.md
     *        +-- source.c
     *        +-- source.h
     *        +-- source.o
     *        +-- source.cpp
     *        +-- source.hpp
     */

    //////////////////////////////////////////////////////////////////////////
    // Check that we can find all files with a relative path glob pattern.
    //////////////////////////////////////////////////////////////////////////

    char pattern[PATH_MAX];
    int ret = snprintf(pattern, PATH_MAX, "%s/**/*", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);

    StringSet *matches = GlobFileList(pattern);

    const char *filename = NULL;
    StringSetIterator iter = StringSetIteratorInit(matches);
    while ((filename = StringSetIteratorNext(&iter)) != NULL) {
        printf(" -> %s\n", filename);
    }

    assert_int_equal(
        StringSetSize(matches), num_test_files + num_test_subdirs);

    for (size_t i = 0; i < num_test_files; i++)
    {
        const int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s",
            test_dir,
            test_files[i]);
        assert_true(ret < PATH_MAX && ret >= 0);
        assert_true(StringSetContains(matches, path));
    }

    for (size_t i = 0; i < num_test_subdirs; i++)
    {
        const int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s",
            test_dir,
            test_subdirs[i]);
        assert_true(ret < PATH_MAX && ret >= 0);
        assert_true(StringSetContains(matches, path));
    }

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Check that we can find all files with an absolute path glob pattern.
    //////////////////////////////////////////////////////////////////////////

    char cwd[PATH_MAX];
    assert_true(getcwd(cwd, PATH_MAX) != NULL);

    ret = snprintf(pattern, PATH_MAX, "%s/%s/**/*", cwd, test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);

    matches = GlobFileList(pattern);
    assert_int_equal(
        StringSetSize(matches), num_test_files + num_test_subdirs);

    for (size_t i = 0; i < num_test_files; i++)
    {
        const int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s" FILE_SEPARATOR_STR "%s",
            cwd,
            test_dir,
            test_files[i]);
        assert_true(ret < PATH_MAX && ret >= 0);
        assert_true(StringSetContains(matches, path));
    }

    for (size_t i = 0; i < num_test_subdirs; i++)
    {
        const int ret = snprintf(
            path,
            PATH_MAX,
            "%s" FILE_SEPARATOR_STR "%s" FILE_SEPARATOR_STR "%s",
            cwd,
            test_dir,
            test_subdirs[i]);
        assert_true(ret < PATH_MAX && ret >= 0);
        assert_true(StringSetContains(matches, path));
    }

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find 'foo', 'bar', 'baz', 'qux' using brace expansion
    //////////////////////////////////////////////////////////////////////////

#ifdef WITH_PCRE2 // POSIX glob(3) does not support glob expansion.
    ret = snprintf(pattern, PATH_MAX, "%s/**/{foo,bar,baz,qux}", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);
    assert_int_equal(StringSetSize(matches), 4);

    ret = snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_STR "foo", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR "bar",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "baz",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_STR "qux", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);
#endif // WITH_PCRE2

    //////////////////////////////////////////////////////////////////////////
    // Let's find all markdown files
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(pattern, PATH_MAX, "%s/**/*.md", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 5);

    ret = snprintf(
        path, PATH_MAX, "%s" FILE_SEPARATOR_STR "README.md", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR "README.md",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "README.md",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "baz" FILE_SEPARATOR_STR "README.md",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "README.md",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find all one letter extension files
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(pattern, PATH_MAX, "%s/**/*.?", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 3);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "source.c",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "source.h",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "source.o",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find all C++ files
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(pattern, PATH_MAX, "%s/qux/source.[ch]pp", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 2);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "source.cpp",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "source.hpp",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find const is {love,life} files
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(
        pattern, PATH_MAX, "%s/foo/bar/baz/const is l[oi][vf]e", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 2);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "baz" FILE_SEPARATOR_STR "const is love",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "baz" FILE_SEPARATOR_STR "const is life",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find dot files
    //////////////////////////////////////////////////////////////////////////

#ifdef WITH_PCRE2
    /* POSIX glob(3) does not support GLOB_PERIOD, meaning it will not find
     * this file, thus cause tests below to fail. */
    ret = snprintf(pattern, PATH_MAX, "%s/**/.*", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 1);

    ret = snprintf(
        path, PATH_MAX, "%s" FILE_SEPARATOR_STR ".gitignore", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);
#endif // WITH_PCRE2

    //////////////////////////////////////////////////////////////////////////
    // Let's use ranges
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(pattern, PATH_MAX, "%s/**/[b-f][a-o][o-y]", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 2);

    ret = snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_STR "foo", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR "bar",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's find exact match
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(pattern, PATH_MAX, "%s/foo/bank_statements.txt", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 1);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR "bank_statements.txt",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);

    //////////////////////////////////////////////////////////////////////////
    // Let's combine a whole bunch of different wildcards
    //////////////////////////////////////////////////////////////////////////

#ifdef WITH_PCRE2 // POSIX glob(3) does not support brace expansion.
    ret = snprintf(
        pattern,
        PATH_MAX,
        "%s/{foo,bar,baz}/[bf][!b-z]{r,z}/**/*c[r]e?.?p?",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 1);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        "bar" FILE_SEPARATOR_STR "secret.gpg",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);
#endif // WITH_PCRE2

    //////////////////////////////////////////////////////////////////////////
    // Let's use the '.' and '..' directory entries in the pattern.
    //////////////////////////////////////////////////////////////////////////

    ret = snprintf(
        pattern, PATH_MAX, "%s/foo/../qux/../qux/./source.o", test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    matches = GlobFileList(pattern);

    assert_int_equal(StringSetSize(matches), 1);

    ret = snprintf(
        path,
        PATH_MAX,
        "%s" FILE_SEPARATOR_STR "foo" FILE_SEPARATOR_STR
        ".." FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR
        ".." FILE_SEPARATOR_STR "qux" FILE_SEPARATOR_STR "." FILE_SEPARATOR_STR
        "source.o",
        test_dir);
    assert_true(ret < PATH_MAX && ret >= 0);
    assert_true(StringSetContains(matches, path));

    StringSetDestroy(matches);
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] = {
        unit_test(test_expand_braces),
        unit_test(test_normalize_path),
#ifdef WITH_PCRE2
        unit_test(test_translate_bracket),
        unit_test(test_translate_glob),
        unit_test(test_glob_match),
        unit_test(test_glob_find),
#endif // WITH_PCRE2
        unit_test(test_glob_file_list),
    };

    return run_tests(tests);
}
