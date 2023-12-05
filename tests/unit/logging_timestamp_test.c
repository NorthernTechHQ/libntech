#include <platform.h>
#include <definitions.h>
#include <test.h>
#include <logging.h>

#ifdef WITH_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#endif

static void test_timestamp_regex(void)
{
    LoggingSetAgentType("test");
    LoggingEnableTimestamps(true);
    LoggingSetColor(false);
    fflush(stderr);
    fflush(stdout);
    int pipe_fd[2];
    assert_int_equal(pipe(pipe_fd), 0);
    // Duplicate stdout.
    int duplicate_stdout = dup(1);
    assert_true(duplicate_stdout >= 0);
    // Make stderr point to the pipe.
    assert_int_equal(dup2(pipe_fd[1], 1), 1);
    Log(LOG_LEVEL_ERR, "Test string");
    fputc('\n', stdout); /* Make sure fgets() doesn't hang. */
    fflush(stderr);
    fflush(stdout);
    // Restore stdout.
    assert_int_equal(dup2(duplicate_stdout, 1), 1);

    char buf[CF_BUFSIZE];
    FILE *pipe_read_end = fdopen(pipe_fd[0], "r");
    assert_true(pipe_read_end != NULL);
    assert_true(fgets(buf, sizeof(buf), pipe_read_end) != NULL);

#ifdef WITH_PCRE2
    int err_code;
    size_t err_offset;
    pcre2_code *regex = pcre2_compile(LOGGING_TIMESTAMP_REGEX, PCRE2_ZERO_TERMINATED,
                                      PCRE2_MULTILINE, &err_code, &err_offset, NULL);
    assert_true(regex != NULL);
    pcre2_match_data *md = pcre2_match_data_create(0, NULL);
    assert_true(pcre2_match(regex, (PCRE2_SPTR) buf, PCRE2_ZERO_TERMINATED, 0, 0, md, NULL) >= 1);
    pcre2_match_data_free(md);
    pcre2_code_free(regex);
#endif // WITH_PCRE2

    fclose(pipe_read_end);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    close(duplicate_stdout);
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_timestamp_regex),
    };

    return run_tests(tests);
}
