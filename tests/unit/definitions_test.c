#include <platform.h>
#include <definitions.h>

#include <test.h>

static void test_metric_prefixes(void)
{
    size_t size;

    size = KIBIBYTE(0);
    assert_int_equal(size, 0);
    size = KIBIBYTE(1);
    assert_int_equal(size, 1024);
    size = KIBIBYTE(2);
    assert_int_equal(size, 2048);

    size = MEBIBYTE(0);
    assert_int_equal(size, 0);
    size = MEBIBYTE(1);
    assert_int_equal(size, 1048576);
    size = MEBIBYTE(2);
    assert_int_equal(size, 2097152);

    size = GIBIBYTE(0);
    assert_int_equal(size, 0);
    size = GIBIBYTE(1);
    assert_int_equal(size, 1073741824);
    size = GIBIBYTE(2);
    assert_int_equal(size, 2147483648);

    size = TEBIBYTE(0);
    assert_int_equal(size, 0);
    size = TEBIBYTE(1);
    assert_int_equal(size, 1099511627776);
    size = TEBIBYTE(2);
    assert_int_equal(size, 2199023255552);

    assert_int_equal(KIBIBYTE(1024), MEBIBYTE(1));
    assert_int_equal(MEBIBYTE(1024), GIBIBYTE(1));
    assert_int_equal(GIBIBYTE(1024), TEBIBYTE(1));
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_metric_prefixes)
    };

    return run_tests(tests);
}
