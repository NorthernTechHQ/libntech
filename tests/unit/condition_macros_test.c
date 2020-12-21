#include <test.h>
#include <cmockery.h>

// No easy way to test that compiler macros fail,
// since we expect these unit tests to compile successfully.
// Thus, we only test that they work for the positive case
// (it compiles). You can manually change one of these
// cases and see it fail.

#define ABC "ABC"

static void test_nt_static_assert()
{
    // Basic:
    nt_static_assert(1);

    // Const boolean logic works at compile time:
    nt_static_assert(true);
    nt_static_assert(!false);
    nt_static_assert(false == false);

    // So does arithmetic:
    nt_static_assert(0 + 1);
    nt_static_assert(-1 + 1 + 1 + 0);

    // And comparisons:
    nt_static_assert(1 > 0);
    nt_static_assert(-1 < 0);
    nt_static_assert(10 * 10 > 99);
    nt_static_assert(10 * 10 == 100);

    // sizeof works at compile-time, except for variable length arrays:
    nt_static_assert(sizeof(char) == 1);
    nt_static_assert(sizeof(char) <= sizeof(short));
    nt_static_assert(sizeof(short) <= sizeof(int));
    nt_static_assert(sizeof(int) <= sizeof(long));

    // String literal indices? Doesn't work!
    // nt_static_assert(ABC[0] == 'A');

    // You can use sizeof on arrays:
    char ten[10];
    nt_static_assert(sizeof(ten) == 10);

    // Sizeof does not return the number of elements in array:
    int many[42];
    nt_static_assert((sizeof(many[0]) * 42) == sizeof(many));

    // strlen() can be used statically:
    nt_static_assert(strlen("CFEngine") == strlen("libntech"));
    nt_static_assert(strlen(ABC) == strlen("ABC"));
    nt_static_assert(strlen(ABC) == (sizeof(ABC) - 1));
    nt_static_assert(sizeof(ABC) == (strlen(ABC) + 1));

    // Various stdlib macros and functions work at
    // compile-time when you give them constant inputs:
    // nt_static_assert(strcmp("libntech", "libntech") == 0);
    // nt_static_assert(strcmp("CFEngine", "Mender") != 0);
    // Commented out since strcmp is only optimized out on some compilers :(

    // Add more cases here if you want to see if a given
    // macro or function can be done at compile time,
    // and used in static asserts.
}

static void test_nt_static_assert_string_equal()
{
    // These are commented out because the macro
    // doesn't work everywhere yet. Highly dependent on
    // how smart the compiler is (How much it optimizes out).

    // nt_static_assert_string_equal("", "");
    // nt_static_assert_string_equal("a", "a");
    // nt_static_assert_string_equal("abc", "abc");
    // nt_static_assert_string_equal(ABC, ABC);
    // nt_static_assert_string_equal("ABC", ABC);
    // nt_static_assert_string_equal(ABC, "ABC");
    // nt_static_assert_string_equal(ABC, "ABC");

    // Concatenate string literals:
    // nt_static_assert_string_equal(ABC ABC, "ABCABC");
    // nt_static_assert_string_equal(ABC ABC, ABC ABC);
    // nt_static_assert_string_equal("" ABC, "" ABC);
    // nt_static_assert_string_equal(ABC "", ABC "");
    // nt_static_assert_string_equal("ABC" "ABC", "ABCABC");
    // nt_static_assert_string_equal("ABC" "ABC", "ABC" "ABC");
    // nt_static_assert_string_equal("ABC" "ABC", ABC ABC);
    // nt_static_assert_string_equal(ABC ABC, "ABC" "ABC");
    // nt_static_assert_string_equal(ABC ABC, ABC ABC);
}


int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_nt_static_assert),
        unit_test(test_nt_static_assert_string_equal),
    };

    int ret = run_tests(tests);

    return ret;
}
