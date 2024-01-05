/*
  Copyright 2021 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <test.h>

#include <stdio.h>
#include <unistd.h>
#include <alloc.h>
#include <sequence.h>

#include <proc_manager.h>

#if defined(__ANDROID__)
# define TEMP_DIR "/data/data/com.termux/files/usr/tmp"
#else
# define TEMP_DIR "/tmp/proc_manager_test"
#endif
#define TEST_FILE "proc_manager_test.txt"

static void test_add_get_process_destroy(void)
{
    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 1234, input, output, 'i');
    assert_true(ret);

    Subprocess *proc = ProcManagerGetProcessByPID(manager, 1234);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessById(manager, "test-process");
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByStream(manager, input);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByFD(manager, fileno(input));
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    /* lookup_io was 'i', so shouldn't be possible to get it from the *output* stream/FD */
    proc = ProcManagerGetProcessByStream(manager, output);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(output));
    assert_true(proc == NULL);

    fclose(input);
    fclose(output);

    ProcManagerDestroy(manager);
}

static void test_add_get_process_multi_destroy(void)
{
    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 0xAA, input, output, 'i');
    assert_true(ret);

    /* Add another process with a different spec and lookup_io set to 'o' this time. */
    FILE *input2 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input2 != NULL);
    FILE *output2 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output2 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process2"), xstrdup("/some/command2 arg1 arg2"), xstrdup("test process2"),
                                (pid_t) 0xAB, input2, output2, 'o');
    assert_true(ret);

    /* Add yet another process with a different spec, with a PID that should
     * make a collision in the internal Map by PID */
    FILE *input3 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input3 != NULL);
    FILE *output3 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output3 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process3"), xstrdup("/some/command3 arg1 arg2"), xstrdup("test process3"),
                                (pid_t) 0x010000AB, input3, output3, 'o');
    assert_true(ret);

    Subprocess *proc = ProcManagerGetProcessByPID(manager, 0xAA);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 0xAA);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessById(manager, "test-process");
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 0xAA);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByStream(manager, input);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 0xAA);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByFD(manager, fileno(input));
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 0xAA);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    /* lookup_io was 'i', so shouldn't be possible to get it from the *output* stream/FD */
    proc = ProcManagerGetProcessByStream(manager, output);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(output));
    assert_true(proc == NULL);


    proc = ProcManagerGetProcessByPID(manager, 0xAB);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process2");
    assert_string_equal(proc->cmd, "/some/command2 arg1 arg2");
    assert_string_equal(proc->description, "test process2");
    assert_int_equal(proc->pid, 0xAB);
    assert_true(proc->input == input2);
    assert_true(proc->output == output2);

    proc = ProcManagerGetProcessById(manager, "test-process2");
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process2");
    assert_string_equal(proc->cmd, "/some/command2 arg1 arg2");
    assert_string_equal(proc->description, "test process2");
    assert_int_equal(proc->pid, 0xAB);
    assert_true(proc->input == input2);
    assert_true(proc->output == output2);

    proc = ProcManagerGetProcessByStream(manager, output2);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process2");
    assert_string_equal(proc->cmd, "/some/command2 arg1 arg2");
    assert_string_equal(proc->description, "test process2");
    assert_int_equal(proc->pid, 0xAB);
    assert_true(proc->input == input2);
    assert_true(proc->output == output2);

    proc = ProcManagerGetProcessByFD(manager, fileno(output2));
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process2");
    assert_string_equal(proc->cmd, "/some/command2 arg1 arg2");
    assert_string_equal(proc->description, "test process2");
    assert_int_equal(proc->pid, 0xAB);
    assert_true(proc->input == input2);
    assert_true(proc->output == output2);

    /* lookup_io was 'o', so shouldn't be possible to get it from the *input* stream/FD */
    proc = ProcManagerGetProcessByStream(manager, input2);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(input2));
    assert_true(proc == NULL);


    proc = ProcManagerGetProcessByPID(manager, 0x010000AB);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process3");
    assert_string_equal(proc->cmd, "/some/command3 arg1 arg2");
    assert_string_equal(proc->description, "test process3");
    assert_int_equal(proc->pid, 0x010000AB);
    assert_true(proc->input == input3);
    assert_true(proc->output == output3);

    proc = ProcManagerGetProcessById(manager, "test-process3");
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process3");
    assert_string_equal(proc->cmd, "/some/command3 arg1 arg2");
    assert_string_equal(proc->description, "test process3");
    assert_int_equal(proc->pid, 0x010000AB);
    assert_true(proc->input == input3);
    assert_true(proc->output == output3);

    proc = ProcManagerGetProcessByStream(manager, output3);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process3");
    assert_string_equal(proc->cmd, "/some/command3 arg1 arg2");
    assert_string_equal(proc->description, "test process3");
    assert_int_equal(proc->pid, 0x010000AB);
    assert_true(proc->input == input3);
    assert_true(proc->output == output3);

    proc = ProcManagerGetProcessByFD(manager, fileno(output3));
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process3");
    assert_string_equal(proc->cmd, "/some/command3 arg1 arg2");
    assert_string_equal(proc->description, "test process3");
    assert_int_equal(proc->pid, 0x010000AB);
    assert_true(proc->input == input3);
    assert_true(proc->output == output3);

    /* lookup_io was 'o', so shouldn't be possible to get it from the *input* stream/FD */
    proc = ProcManagerGetProcessByStream(manager, input3);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(input3));
    assert_true(proc == NULL);


    fclose(input);
    fclose(output);
    fclose(input2);
    fclose(output2);
    fclose(input3);
    fclose(output3);

    ProcManagerDestroy(manager);
}

static void test_pop(void)
{
    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 1234, input, output, 'i');
    assert_true(ret);

    Subprocess *proc = ProcManagerPopProcessByPID(manager, 1234);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    SubprocessDestroy(proc);

    /* The process should no longer be there. */
    proc = ProcManagerGetProcessById(manager, "test-process");
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByStream(manager, input);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(input));
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(output));
    assert_true(proc == NULL);

    fclose(input);
    fclose(output);

    ProcManagerDestroy(manager);
}

static void test_add_process_twice(void)
{
    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 1234, input, output, 'i');
    assert_true(ret);

    /* Now try to insert the process again (with a different command and
     * description). This shouldn't be allowed. */
    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process"), xstrdup("/some/command2 arg1 arg2"), xstrdup("better test process"),
                                (pid_t) 1234, input, output, 'i');
    assert_false(ret);

    /* But the original process should still be there. */
    Subprocess *proc = ProcManagerGetProcessByPID(manager, 1234);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessById(manager, "test-process");
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByStream(manager, input);
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    proc = ProcManagerGetProcessByFD(manager, fileno(input));
    assert_true(proc != NULL);
    assert_string_equal(proc->id, "test-process");
    assert_string_equal(proc->cmd, "/some/command arg1 arg2");
    assert_string_equal(proc->description, "test process");
    assert_int_equal(proc->pid, 1234);
    assert_true(proc->input == input);
    assert_true(proc->output == output);

    /* lookup_io was 'i', so shouldn't be possible to get it from the *output* stream/FD */
    proc = ProcManagerGetProcessByStream(manager, output);
    assert_true(proc == NULL);
    proc = ProcManagerGetProcessByFD(manager, fileno(output));
    assert_true(proc == NULL);

    fclose(input);
    fclose(output);

    ProcManagerDestroy(manager);
}

void ItemDestroy_fclose(void *data)
{
    FILE *stream = data;
    fclose(stream);
}

static void test_add_get_process_many_destroy(void)
{
    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    const size_t n_procs = 150;
    Seq *inputs = SeqNew(n_procs, ItemDestroy_fclose);
    Seq *outputs = SeqNew(n_procs, ItemDestroy_fclose);
    Seq *ids = SeqNew(n_procs, NULL);

    for (size_t i = 0; i < n_procs; i++)
    {
        FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
        assert_true(input != NULL);
        SeqAppend(inputs, input);
        FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
        assert_true(output != NULL);
        SeqAppend(outputs, output);
        char *id = NULL;
        xasprintf(&id, "test-process%zd", i);
        assert_true(id != NULL);
        SeqAppend(ids, id);

        bool ret = ProcManagerAddProcess(manager,
                                         id, xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                         (pid_t) 1234 + i, input, output, (i % 2) == 0 ? 'i' : 'o');
        assert_true(ret);
    }

    for (size_t i = 0; i < n_procs; i++)
    {
        Subprocess *proc = ProcManagerGetProcessByPID(manager, (pid_t) 1234 + i);
        assert_true(proc != NULL);
        assert_string_equal(proc->id, SeqAt(ids, i));
        assert_string_equal(proc->cmd, "/some/command arg1 arg2");
        assert_string_equal(proc->description, "test process");
        assert_int_equal(proc->pid, (pid_t) 1234 + i);
        assert_true(proc->input == SeqAt(inputs, i));
        assert_true(proc->output == SeqAt(outputs, i));

        proc = ProcManagerGetProcessById(manager, SeqAt(ids, i));
        assert_true(proc != NULL);
        assert_string_equal(proc->id, SeqAt(ids, i));
        assert_string_equal(proc->cmd, "/some/command arg1 arg2");
        assert_string_equal(proc->description, "test process");
        assert_int_equal(proc->pid, (pid_t) 1234 + i);
        assert_true(proc->input == SeqAt(inputs, i));
        assert_true(proc->output == SeqAt(outputs, i));

        proc = ProcManagerGetProcessByStream(manager, (i % 2) == 0 ? SeqAt(inputs, i) : SeqAt(outputs, i));
        assert_true(proc != NULL);
        assert_string_equal(proc->id, SeqAt(ids, i));
        assert_string_equal(proc->cmd, "/some/command arg1 arg2");
        assert_string_equal(proc->description, "test process");
        assert_int_equal(proc->pid, (pid_t) 1234 + i);
        assert_true(proc->input == SeqAt(inputs, i));
        assert_true(proc->output == SeqAt(outputs, i));

        proc = ProcManagerGetProcessByFD(manager, fileno((i % 2) == 0 ? SeqAt(inputs, i) : SeqAt(outputs, i)));
        assert_true(proc != NULL);
        assert_string_equal(proc->id, SeqAt(ids, i));
        assert_string_equal(proc->cmd, "/some/command arg1 arg2");
        assert_string_equal(proc->description, "test process");
        assert_int_equal(proc->pid, (pid_t) 1234 + i);
        assert_true(proc->input == SeqAt(inputs, i));
        assert_true(proc->output == SeqAt(outputs, i));

        proc = ProcManagerGetProcessByStream(manager, (i % 2) == 0 ? SeqAt(outputs, i) : SeqAt(inputs, i));
        assert_true(proc == NULL);
        proc = ProcManagerGetProcessByFD(manager, fileno((i % 2) == 0 ? SeqAt(outputs, i) : SeqAt(inputs, i)));
        assert_true(proc == NULL);
    }

    SeqDestroy(inputs);
    SeqDestroy(outputs);
    SeqDestroy(ids);

    ProcManagerDestroy(manager);
}

static bool TestTerminator(Subprocess *proc, void *data)
{
    int *terminator_call_counter = data;
    (*terminator_call_counter)++;

    /* just close the descriptor, we have fake data, no real process(es) */
    fclose(proc->input);
    fclose(proc->output);

    return true;
}

static void test_terminate(void)
{
    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 1234, input, output, 'i');
    assert_true(ret);

    FILE *input2 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input2 != NULL);
    FILE *output2 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output2 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process2"), xstrdup("/some/command2 arg1 arg2"), xstrdup("test process 2"),
                                (pid_t) 1235, input2, output2, 'i');
    assert_true(ret);

    FILE *input3 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input3 != NULL);
    FILE *output3 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output3 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process3"), xstrdup("/some/command3 arg1 arg2"), xstrdup("test process 3"),
                                (pid_t) 1236, input3, output3, 'i');
    assert_true(ret);

    FILE *input4 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input4 != NULL);
    FILE *output4 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output4 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process4"), xstrdup("/some/command4 arg1 arg2"), xstrdup("test process 4"),
                                (pid_t) 1237, input4, output4, 'i');
    assert_true(ret);

    int terminator_call_counter = 0;
    ret = ProcManagerTerminateProcessByPID(manager, 1234, TestTerminator, &terminator_call_counter);
    assert_true(ret);
    assert_int_equal(terminator_call_counter, 1);

    ret = ProcManagerTerminateProcessById(manager, "test-process2", TestTerminator, &terminator_call_counter);
    assert_true(ret);
    assert_int_equal(terminator_call_counter, 2);

    ret = ProcManagerTerminateProcessByStream(manager, input3, TestTerminator, &terminator_call_counter);
    assert_true(ret);
    assert_int_equal(terminator_call_counter, 3);

    ret = ProcManagerTerminateProcessByFD(manager, fileno(input4), TestTerminator, &terminator_call_counter);
    assert_true(ret);
    assert_int_equal(terminator_call_counter, 4);

    ProcManagerDestroy(manager);
}

static void test_terminate_all(void)
{
    ProcManager *manager = ProcManagerNew();
    assert_true(manager != NULL);

    /* We just need some FILE streams. */
    FILE *input = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input != NULL);
    FILE *output = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output != NULL);

    bool ret = ProcManagerAddProcess(manager,
                                     xstrdup("test-process"), xstrdup("/some/command arg1 arg2"), xstrdup("test process"),
                                     (pid_t) 1234, input, output, 'i');
    assert_true(ret);

    FILE *input2 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input2 != NULL);
    FILE *output2 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output2 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process2"), xstrdup("/some/command2 arg1 arg2"), xstrdup("test process 2"),
                                (pid_t) 1235, input2, output2, 'i');
    assert_true(ret);

    FILE *input3 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input3 != NULL);
    FILE *output3 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output3 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process3"), xstrdup("/some/command3 arg1 arg2"), xstrdup("test process 3"),
                                (pid_t) 1236, input3, output3, 'i');
    assert_true(ret);

    FILE *input4 = fopen(TEMP_DIR"/"TEST_FILE, "w");
    assert_true(input4 != NULL);
    FILE *output4 = fopen(TEMP_DIR"/"TEST_FILE, "r");
    assert_true(output4 != NULL);

    ret = ProcManagerAddProcess(manager,
                                xstrdup("test-process4"), xstrdup("/some/command4 arg1 arg2"), xstrdup("test process 4"),
                                (pid_t) 1237, input4, output4, 'i');
    assert_true(ret);

    int terminator_call_counter = 0;
    ret = ProcManagerTerminateAllProcesses(manager, TestTerminator, &terminator_call_counter);
    assert_true(ret);
    assert_int_equal(terminator_call_counter, 4);

    ProcManagerDestroy(manager);
}

static void init(void)
{
    int ret = mkdir(TEMP_DIR, 0755);
    if (ret != 0)
    {
        printf("Failed to create directory for tests!\n");
        exit(EXIT_FAILURE);
    }

    ret = open(TEMP_DIR "/" TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ret == -1)
    {
        printf("Failed to create test file for tests!\n");
        exit(EXIT_FAILURE);
    }
}

static void cleanup(void)
{
    unlink(TEMP_DIR"/"TEST_FILE);
    rmdir(TEMP_DIR);
}

int main()
{
    PRINT_TEST_BANNER();

    init();

    const UnitTest tests[] = {
            unit_test(test_add_get_process_destroy),
            unit_test(test_add_get_process_multi_destroy),
            unit_test(test_pop),
            unit_test(test_add_process_twice),
            unit_test(test_add_get_process_many_destroy),
            unit_test(test_terminate),
            unit_test(test_terminate_all),
    };

    int ret = run_tests(tests);

    cleanup();
    return ret;
}

