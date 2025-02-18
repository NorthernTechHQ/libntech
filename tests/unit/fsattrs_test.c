#include <test.h>
#include <cmockery.h>
#include <fsattrs.h>
#include <stdlib.h>


static void test_file_immutable_flag(void)
{
    /* Get immutable flag from non existent file */
    unlink("no_such_file_639123");
    bool flag;
    FSAttrsResult res = FileGetImmutableFlag("no_such_file_639123", &flag);
    assert_int_not_equal(res, FS_ATTRS_SUCCESS);
    assert_int_not_equal(res, FS_ATTRS_FAILURE);

    /* Make temporary test file */
    char filename[] = "test_file_immutable_flag_XXXXXX";
    int fd = mkstemp(filename);
    assert_int_not_equal(fd, -1);
    close(fd);

    /* There should not be any immutable flag on a newly created file */
    res = FileGetImmutableFlag(filename, &flag);
    unlink(filename);
    if (res == FS_ATTRS_SUCCESS)
    {
        assert_false(flag);
    }
    assert_int_not_equal(res, FS_ATTRS_FAILURE);
    assert_int_not_equal(res, FS_ATTRS_NOENTRY);

    /* It would be nice to test updating inode flags, however this required
       root permissions */
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_file_immutable_flag),
    };

    int ret = run_tests(tests);
    return ret;
}
