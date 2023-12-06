#include <platform.h>

#ifdef WITH_PCRE2
#include <regex.h>
#else
#error "PCRE2 required for regex tests"
#endif

#include <buffer.h>

#include <test.h>

static void test_match(void)
{
    assert_true(StringMatch("^a.*$", "abc", NULL, NULL));
    assert_true(StringMatch("a", "a", NULL, NULL));
    assert_true(StringMatch("a", "ab", NULL, NULL));
    assert_false(StringMatch("^a.*$", "bac", NULL, NULL));

    size_t start, end;
    bool ret = StringMatch("[a-z]{3}", "abc", &start, &end);
    assert_true(ret);
    assert_int_equal(start, 0);
    assert_int_equal(end, 3);

    ret = StringMatch("^[a-z]{3}$", "abc", &start, &end);
    assert_true(ret);
    assert_int_equal(start, 0);
    assert_int_equal(end, 3);

    ret = StringMatch("^[a-z]{3}.*$", "abcdef", &start, &end);
    assert_true(ret);
    assert_int_equal(start, 0);
    assert_int_equal(end, 6);

    ret = StringMatch("[a-z]{3}.*", "0abcdef", &start, &end);
    assert_true(ret);
    assert_int_equal(start, 1);
    assert_int_equal(end, 7);
}


static void test_match_full(void)
{
    assert_true(StringMatchFull("^a.*$", "abc"));
    assert_true(StringMatchFull("a", "a"));
    assert_false(StringMatchFull("a", "ab"));
    assert_false(StringMatchFull("^a.*$", "bac"));
}


static void test_match_with_captures(void)
{
    Seq *ret = StringMatchCaptures("^a(.).*$", "abc", false);
    assert_true(ret);
    assert_int_equal(SeqLength(ret), 2);
    assert_string_equal(BufferData(SeqAt(ret, 0)), "abc");
    assert_string_equal(BufferData(SeqAt(ret, 1)), "b");
    SeqDestroy(ret);

    ret = StringMatchCaptures("^a(.).*$", "abc", true);
    assert_true(ret);
    assert_int_equal(SeqLength(ret), 4);
    assert_string_equal(BufferData(SeqAt(ret, 0)), "0");
    assert_string_equal(BufferData(SeqAt(ret, 1)), "abc");
    assert_string_equal(BufferData(SeqAt(ret, 2)), "1");
    assert_string_equal(BufferData(SeqAt(ret, 3)), "b");
    SeqDestroy(ret);

    ret = StringMatchCaptures("^a(?<mid>..)d*$", "abcd", false);
    assert_true(ret);
    assert_int_equal(SeqLength(ret), 2);
    assert_string_equal(BufferData(SeqAt(ret, 0)), "abcd");
    assert_string_equal(BufferData(SeqAt(ret, 1)), "bc");
    SeqDestroy(ret);

    ret = StringMatchCaptures("^a(?<mid>..)d*$", "abcd", true);
    assert_true(ret);
    assert_int_equal(SeqLength(ret), 4);
    assert_string_equal(BufferData(SeqAt(ret, 0)), "0");
    assert_string_equal(BufferData(SeqAt(ret, 1)), "abcd");
    assert_string_equal(BufferData(SeqAt(ret, 2)), "mid");
    assert_string_equal(BufferData(SeqAt(ret, 3)), "bc");
    SeqDestroy(ret);
}

void test_search_and_replace(void)
{
    Buffer *buf = BufferNewFrom("abcd", 4);
    const char *err = BufferSearchAndReplace(buf, "b", "B", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "aBcd");

    err = BufferSearchAndReplace(buf, "cd$", "CDef", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "aBCDef");

    err = BufferSearchAndReplace(buf, "([A-Z]{2})([a-z]{2})", "$2$1", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "aBefCD");

    err = BufferSearchAndReplace(buf, "([a-z]{2})([A-Z]{2})", "\\2\\1", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "aBCDef");

    err = BufferSearchAndReplace(buf, "abcdef", "abcd", "i");
    assert_false(err);
    assert_string_equal(BufferData(buf), "abcd");

    err = BufferSearchAndReplace(buf, "bc", "$`$'", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "aadd");

    err = BufferSearchAndReplace(buf, "aadd", "$`$'abcd", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "abcd");

    err = BufferSearchAndReplace(buf, "a([a-z])([a-z])d", "a$2$1d [$+]", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "acbd [2]");

    err = BufferSearchAndReplace(buf, "a([a-z])([a-z])d", "a$2$1d [$&]", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "abcd [acbd] [2]");

    err = BufferSearchAndReplace(buf, "a", "A", "g");
    assert_false(err);
    assert_string_equal(BufferData(buf), "Abcd [Acbd] [2]");

    err = BufferSearchAndReplace(buf, "(\\w+).*", "$1", "");
    assert_false(err);
    assert_string_equal(BufferData(buf), "Abcd");

    BufferDestroy(buf);
}


int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_match),
        unit_test(test_match_full),
        unit_test(test_match_with_captures),
        unit_test(test_search_and_replace),
    };

    return run_tests(tests);
}
