#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <alloc.h>

#include <proc_manager.h>

/**
 * This file contains ProcManager tests that need to fork() a child process
 * which totally confuses the unit test framework used by the other tests.
 */

static bool failure = false;

#define assert_int_equal(expr1, expr2) \
    if ((expr1) != (expr2))            \
    { \
        fprintf(stderr, "FAIL: "#expr1" != "#expr2" [%d != %d] (%s:%d)\n", (expr1), (expr2), __FILE__, __LINE__); \
        failure = true; \
    }

#define assert_true(expr) \
    if (!(expr)) \
    { \
        fprintf(stderr, "FAIL: "#expr" is FALSE (%s:%d)\n", __FILE__, __LINE__); \
        failure = true; \
    }

#define assert_false(expr) \
    if ((expr)) \
    { \
        fprintf(stderr, "FAIL: "#expr" is TRUE (%s:%d)\n", __FILE__, __LINE__); \
        failure = true; \
    }

static bool TerminateWithSIGTERM(Subprocess *proc, void *data)
{
    assert_true(proc->pid > 0);

    /* Not closing the pipes here, the force-termination should do that after
     * this graceful termination fails. */
    int ret = kill(proc->pid, SIGTERM);
    assert_int_equal(ret, 0);

    /* Give child time to terminate (it shouldn't). */
    sleep(1);
    ret = waitpid(proc->pid, NULL, WNOHANG);
    assert_int_equal(ret, 0);

    bool *failed = data;
    *failed = true;

    /* Attempt failed, leave the process to the force-termination. */
    return false;
}

int main()
{
    int parent_write_child_read[2];
    int ret = pipe(parent_write_child_read);
    assert_int_equal(ret, 0);
    int parent_read_child_write[2];
    ret = pipe(parent_read_child_write);
    assert_int_equal(ret, 0);

    pid_t pid = fork();
    assert_true(pid >= 0);

    if (pid == 0)
    {
        /* child */
        close(parent_write_child_read[1]);
        close(parent_read_child_write[0]);

        /* Also close the ends the child should normally use, we don't need them. */
        close(parent_write_child_read[0]);
        close(parent_read_child_write[1]);

        /* Ignore SIGTERM */
        signal(SIGTERM, SIG_IGN);

        /* Wait to be killed. */
        sleep(3600);
    }
    /* parent just continues below */

    close(parent_write_child_read[0]);
    close(parent_read_child_write[1]);

    /* Give child time to start ignoring SIGTERM */
    sleep(1);

    FILE *input = fdopen(parent_write_child_read[1], "w");
    assert_true(input != NULL);
    FILE *output = fdopen(parent_read_child_write[0], "r");
    assert_true(output != NULL);

    ProcManager *manager = ProcManagerNew();
    bool success = ProcManagerAddProcess(manager,
                                         xstrdup("test-process"), xstrdup("/some/cmd arg1"), xstrdup("test process"),
                                         pid, input, output, 'o');
    assert_true(success);

    bool graceful_termination_failed = false;;
    success = ProcManagerTerminateProcessByFD(manager, fileno(output),
                                              TerminateWithSIGTERM, &graceful_termination_failed);
    assert_true(success);
    assert_true(graceful_termination_failed);

    ret = kill(pid, 0);
    assert_int_equal(ret, -1);  /* there should be no such process anymore */

    ProcManagerDestroy(manager);

    if (failure)
    {
        fprintf(stderr, "FAILED\n");
        return 1;
    }
    else
    {
        fprintf(stderr, "SUCCESS\n");
        return 0;
    }
}
