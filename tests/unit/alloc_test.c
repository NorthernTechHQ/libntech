#include <test.h>

#include <alloc.h>

#include <setjmp.h>
#include <cmockery.h>
#include <stdarg.h>

void test_xasprintf(void)
{
    char *s;
    int res = xasprintf(&s, "Foo%d%s", 123, "17");

    assert_int_equal(res, 8);
    assert_string_equal(s, "Foo12317");
    free(s);
}

void test_xvasprintf_sub(const char *fmt, ...)
{
    char *s;
    va_list ap;

    va_start(ap, fmt);
    int res = xvasprintf(&s, fmt, ap);

    va_end(ap);
    assert_int_equal(res, 8);
    assert_string_equal(s, "Foo12317");
    free(s);
}

void test_xvasprintf(void)
{
    test_xvasprintf_sub("Foo%d%s", 123, "17");
}

void test_free_array_items(void)
{
    char **arr = xcalloc(10, sizeof(char*));
    for (size_t i = 0; i < 10; i++)
    {
        arr[i] = xstrdup("some string");
    }

    free_array_items((void**)arr, 10);
    free(arr);
    /* There should be no memleaks now. */
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_xasprintf),
        unit_test(test_xvasprintf),
    };

    return run_tests(tests);
}
