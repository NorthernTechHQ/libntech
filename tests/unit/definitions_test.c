#include <platform.h>
#include <definitions.h>

#include <test.h>

static void test_metric_prefixes(void)
{
    uint32_t size32;

    size32 = KIBIBYTE(0);
    assert_int_equal(size32, 0UL);
    size32 = KIBIBYTE(1);
    assert_int_equal(size32, 1024UL);
    size32 = KIBIBYTE(2);
    assert_int_equal(size32, 2048UL);

    size32 = MEBIBYTE(0);
    assert_int_equal(size32, 0UL);
    size32 = MEBIBYTE(1);
    assert_int_equal(size32, 1048576UL);
    size32 = MEBIBYTE(2);
    assert_int_equal(size32, 2097152UL);

    uint64_t size64;

    size64 = GIBIBYTE(0);
    assert_int_equal(size64, 0ULL);
    size64 = GIBIBYTE(1);
    assert_int_equal(size64, 1073741824ULL);
    size64 = GIBIBYTE(2);
    assert_int_equal(size64, 2147483648ULL);

    size64 = TEBIBYTE(0);
    assert_int_equal(size64, 0ULL);
    size64 = TEBIBYTE(1);
    assert_int_equal(size64, 1099511627776ULL);
    size64 = TEBIBYTE(2);
    assert_int_equal(size64, 2199023255552ULL);

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
