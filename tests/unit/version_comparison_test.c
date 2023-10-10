#include <version_comparison.h>
#include <test.h>

static void test_CompareVersion(void)
{
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3.15.0"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3.15.0a"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3.15.0a1"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15", "3.15.0"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3.15"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3"));
    assert_true(VERSION_EQUAL == CompareVersion("3", "3.15.0"));
    assert_true(VERSION_EQUAL == CompareVersion("3.", "3"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0", "3.15.0-2"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0-1", "3.15.0-2"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0.123", "3.15.0-2"));
    assert_true(VERSION_EQUAL == CompareVersion("3.15.0.123", "3.15.0.321"));

    assert_true(VERSION_GREATER == CompareVersion("4", "3"));
    assert_true(VERSION_GREATER == CompareVersion("3", "1"));
    assert_true(VERSION_GREATER == CompareVersion("4", "3.15"));
    assert_true(VERSION_GREATER == CompareVersion("4", "3.999"));
    assert_true(VERSION_GREATER == CompareVersion("4", "3.999.999"));
    assert_true(VERSION_GREATER == CompareVersion("4", "3.999.999-999"));
    assert_true(VERSION_GREATER == CompareVersion("3.16", "3.15"));
    assert_true(VERSION_GREATER == CompareVersion("3.16.0", "3.15.999"));
    assert_true(VERSION_GREATER == CompareVersion("3.16.0", "3.15"));
    assert_true(VERSION_GREATER == CompareVersion("3.15.1", "3.15.0"));
    assert_true(VERSION_GREATER == CompareVersion("3.15.10", "3.15.9"));
    assert_true(VERSION_GREATER == CompareVersion("3.10", "3.1"));
    assert_true(VERSION_GREATER == CompareVersion("3.100", "3.1"));

    assert_true(VERSION_SMALLER == CompareVersion("3", "4"));
    assert_true(VERSION_SMALLER == CompareVersion("1", "3"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15", "4"));
    assert_true(VERSION_SMALLER == CompareVersion("3.999", "4"));
    assert_true(VERSION_SMALLER == CompareVersion("3.999.999", "4"));
    assert_true(VERSION_SMALLER == CompareVersion("3.999.999-999", "4"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15", "3.16"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15.999", "3.16.0"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15", "3.16.0"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15.0", "3.15.1"));
    assert_true(VERSION_SMALLER == CompareVersion("3.15.9", "3.15.10"));
    assert_true(VERSION_SMALLER == CompareVersion("3.1", "3.10"));
    assert_true(VERSION_SMALLER == CompareVersion("3.1", "3.100"));

    assert_true(VERSION_ERROR == CompareVersion("", ""));
    assert_true(VERSION_ERROR == CompareVersion("1", ""));
    assert_true(VERSION_ERROR == CompareVersion("", "1"));
    assert_true(VERSION_ERROR == CompareVersion("", "3.16.0"));
}

static void test_CompareVersionExpression(void)
{
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("1.2.3", "=", "1.2.3"));
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("1.2.3", "==", "1.2.3"));
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("1.2.3", "!=", "1.2.4"));
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("100.0.0", ">", "99.0.0"));
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("100.0.0", ">=", "99.0.0"));
    assert_true(BOOLEAN_TRUE == CompareVersionExpression("99.88.77", "<", "999.0.0"));

    assert_true(BOOLEAN_FALSE == CompareVersionExpression("1.2.3", "!=", "1.2.3"));
    assert_true(BOOLEAN_FALSE == CompareVersionExpression("1.2.3", "=", "1.2.4"));
    assert_true(BOOLEAN_FALSE == CompareVersionExpression("1.2.3", "==", "1.2.4"));
    assert_true(BOOLEAN_FALSE == CompareVersionExpression("100.0.0", "<=", "99.0.0"));
    assert_true(BOOLEAN_FALSE == CompareVersionExpression("100.0.0", "<", "99.0.0"));
    assert_true(BOOLEAN_FALSE == CompareVersionExpression("99.88.77", ">=", "999.0.0"));

    assert_true(BOOLEAN_ERROR == CompareVersionExpression("", "", ""));
    assert_true(BOOLEAN_ERROR == CompareVersionExpression("1", "2", "3"));
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_CompareVersion),
        unit_test(test_CompareVersionExpression),
    };

    return run_tests(tests);
}
