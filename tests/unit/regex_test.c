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


int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_match),
        unit_test(test_match_full),
        unit_test(test_match_with_captures),
    };

    return run_tests(tests);
}
