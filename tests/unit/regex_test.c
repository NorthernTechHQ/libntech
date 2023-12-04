#include <platform.h>

#ifdef WITH_PCRE2
#include <regex.h>
#else
#error "PCRE2 required for regex tests"
#endif

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


int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_match),
        unit_test(test_match_full),
    };

    return run_tests(tests);
}
