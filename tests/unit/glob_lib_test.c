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
    /* This test is not very thorough, because we don't want unit tests to
     * create or depend on too many files. More testing will be done through
     * acceptance tests.
     */
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


static void test_glob_file_list(void)
{
    /* This test is not very thorough, because we don't want unit tests to
     * create or depend on too many files. More testing will be done through
     * acceptance tests.
     */
    StringSet *const matches = GlobFileList("test.{h,c}");
    assert_int_equal(StringSetSize(matches), 2);
    assert_true(StringSetContains(matches, "test.c"));
    assert_true(StringSetContains(matches, "test.h"));
    assert_false(StringSetContains(matches, "test.o"));
    StringSetDestroy(matches);
}

#endif // WITH_PCRE2

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
        unit_test(test_glob_file_list),
#endif // WITH_PCRE2
    };

    return run_tests(tests);
}
