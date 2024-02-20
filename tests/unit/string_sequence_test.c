#include <platform.h>
#include <sequence.h>
#include <string_sequence.h>
#include <test.h>


static void test_StringJoin(void)
{
    const char *argv[] = { "one", "two", "three" };
    const int argc = sizeof(argv) / sizeof(argv[0]);

    Seq *seq = SeqFromArgv(argc, argv);

    char *actual = StringJoin(seq, NULL);
    assert_string_equal(actual, "onetwothree");
    free(actual);

    actual = StringJoin(seq, "");
    assert_string_equal(actual, "onetwothree");
    free(actual);

    actual = StringJoin(seq, ", ");
    assert_string_equal(actual, "one, two, three");
    free(actual);

    SeqDestroy(seq);
    seq = SeqNew(0, NULL);

    actual = StringJoin(seq, NULL);
    assert_string_equal(actual, "");
    free(actual);

    actual = StringJoin(seq, "");
    assert_string_equal(actual, "");
    free(actual);

    actual = StringJoin(seq, ", ");
    assert_string_equal(actual, "");
    free(actual);

    SeqDestroy(seq);
}

static void test_StringSplit(void)
{
    {
        Seq *const seq = StringSplit("foo", ",");
        assert_int_equal(SeqLength(seq), 1);
        assert_string_equal(SeqAt(seq, 0), "foo");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("foo,bar", ",");
        assert_int_equal(SeqLength(seq), 2);
        assert_string_equal(SeqAt(seq, 0), "foo");
        assert_string_equal(SeqAt(seq, 1), "bar");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("foo,bar,baz", ",");
        assert_int_equal(SeqLength(seq), 3);
        assert_string_equal(SeqAt(seq, 0), "foo");
        assert_string_equal(SeqAt(seq, 1), "bar");
        assert_string_equal(SeqAt(seq, 2), "baz");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("foo,bar,baz,", ",");
        assert_int_equal(SeqLength(seq), 4);
        assert_string_equal(SeqAt(seq, 0), "foo");
        assert_string_equal(SeqAt(seq, 1), "bar");
        assert_string_equal(SeqAt(seq, 2), "baz");
        assert_string_equal(SeqAt(seq, 3), "");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("", ",");
        assert_int_equal(SeqLength(seq), 1);
        assert_string_equal(SeqAt(seq, 0), "");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("/etc/os-release", "/");
        assert_int_equal(SeqLength(seq), 3);
        assert_string_equal(SeqAt(seq, 0), "");
        assert_string_equal(SeqAt(seq, 1), "etc");
        assert_string_equal(SeqAt(seq, 2), "os-release");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit(".ssh/authorized_keys", "/");
        assert_int_equal(SeqLength(seq), 2);
        assert_string_equal(SeqAt(seq, 0), ".ssh");
        assert_string_equal(SeqAt(seq, 1), "authorized_keys");
        SeqDestroy(seq);
    }
    {
        Seq *const seq =
            StringSplit("C:\\Program Files\\Cfengine/bin/cf-agent.exe", "\\/");
        assert_int_equal(SeqLength(seq), 5);
        assert_string_equal(SeqAt(seq, 0), "C:");
        assert_string_equal(SeqAt(seq, 2), "Cfengine");
        assert_string_equal(SeqAt(seq, 4), "cf-agent.exe");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("/foo//bar\\/baz\\", "\\/");
        assert_int_equal(SeqLength(seq), 7);
        assert_string_equal(SeqAt(seq, 0), "");
        assert_string_equal(SeqAt(seq, 1), "foo");
        assert_string_equal(SeqAt(seq, 2), "");
        assert_string_equal(SeqAt(seq, 3), "bar");
        assert_string_equal(SeqAt(seq, 4), "");
        assert_string_equal(SeqAt(seq, 5), "baz");
        assert_string_equal(SeqAt(seq, 6), "");
        SeqDestroy(seq);
    }
    {
        Seq *const seq = StringSplit("foo", "");
        assert_int_equal(SeqLength(seq), 1);
        assert_string_equal(SeqAt(seq, 0), "foo");
        SeqDestroy(seq);
    }
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_StringJoin),
        unit_test(test_StringSplit),
    };

    return run_tests(tests);
}
