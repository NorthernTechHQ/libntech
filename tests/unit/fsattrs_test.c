#include <test.h>
#include <cmockery.h>
#include <fsattrs.h>
#include <stdlib.h>


static void test_fsattrs_immutable_flag(void)
{
    /* Get immutable flag from non existent file */
    unlink("no_such_file_639123");
    bool flag;
    FSAttrsResult res = FSAttrsGetImmutableFlag("no_such_file_639123", &flag);
    if (res == FS_ATTRS_NOT_SUPPORTED)
    {
        /* We assume it's not supported. Hence, there is no point in executing
         * the remaining test code. */
        return;
    }
    assert_int_equal(res, FS_ATTRS_DOES_NOT_EXIST);

    /* Make temporary test file */
    char filename[] = "test_file_immutable_flag_XXXXXX";
    int fd = mkstemp(filename);
    assert_int_not_equal(fd, -1);
    close(fd);

    /* There should not be any immutable flag on a newly created file */
    res = FSAttrsGetImmutableFlag(filename, &flag);
    unlink(filename);
    if (res == FS_ATTRS_SUCCESS)
    {
        assert_false(flag);
    }
    else
    {
        assert_int_equal(res, FS_ATTRS_NOT_SUPPORTED);
    }

    /* It would be nice to test updating inode flags, however this requires
     * root permissions. And, we don't want to run make check as root. */
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_fsattrs_immutable_flag),
    };

    int ret = run_tests(tests);
    return ret;
}
