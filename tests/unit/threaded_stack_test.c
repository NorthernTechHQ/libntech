#include "test.h"

#include <alloc.h>
#include <threaded_stack.h>

static void test_push_pop(void)
{
    ThreadedStack *stack = ThreadedStackNew(0, free);

    ThreadedStackPush(stack, xstrdup("1"));
    ThreadedStackPush(stack, xstrdup("2"));
    ThreadedStackPush(stack, xstrdup("3"));

    char *str1 = ThreadedStackPop(stack);
    char *str2 = ThreadedStackPop(stack);
    char *str3 = ThreadedStackPop(stack);
    assert_int_equal(strcmp(str1, "3"), 0);
    assert_int_equal(strcmp(str2, "2"), 0);
    assert_int_equal(strcmp(str3, "1"), 0);

    free(str1);
    free(str2);
    free(str3);

    ThreadedStackDestroy(stack);
}

static void test_pop_empty_and_push_null(void)
{
    ThreadedStack *stack = ThreadedStackNew(1, NULL);

    assert(ThreadedStackIsEmpty(stack));

    void *i_am_null = ThreadedStackPop(stack);
    assert(i_am_null == NULL);
    ThreadedStackPush(stack, i_am_null);
    assert(ThreadedStackPop(stack) == NULL);

    ThreadedStackDestroy(stack);
}

static void test_copy(void)
{
    ThreadedStack *stack = ThreadedStackNew(4, free);

    ThreadedStackPush(stack, xstrdup("1"));
    ThreadedStackPush(stack, xstrdup("2"));
    ThreadedStackPush(stack, xstrdup("3"));

    ThreadedStack *new_stack = ThreadedStackCopy(stack);

    assert(new_stack != NULL);
    assert_int_equal(ThreadedStackCount(stack), ThreadedStackCount(new_stack));
    assert_int_equal(ThreadedStackCapacity(stack), ThreadedStackCapacity(new_stack));

    char *old_str1 = ThreadedStackPop(stack); char *new_str1 = ThreadedStackPop(new_stack);
    char *old_str2 = ThreadedStackPop(stack); char *new_str2 = ThreadedStackPop(new_stack);
    char *old_str3 = ThreadedStackPop(stack); char *new_str3 = ThreadedStackPop(new_stack);

    assert(old_str1 == new_str1);
    assert(old_str2 == new_str2);
    assert(old_str3 == new_str3);

    free(old_str1);
    free(old_str2);
    free(old_str3);

    ThreadedStackSoftDestroy(stack);

    // Tests expanding the copied stack
    ThreadedStackPush(new_stack, xstrdup("1"));
    ThreadedStackPush(new_stack, xstrdup("2"));
    ThreadedStackPush(new_stack, xstrdup("3"));
    ThreadedStackPush(new_stack, xstrdup("4"));
    ThreadedStackPush(new_stack, xstrdup("5"));

    assert_int_equal(ThreadedStackCount(new_stack), 5);
    assert_int_equal(ThreadedStackCapacity(new_stack), 8);

    new_str1 = ThreadedStackPop(new_stack);
    new_str2 = ThreadedStackPop(new_stack);
    new_str3 = ThreadedStackPop(new_stack);
    char *new_str4 = ThreadedStackPop(new_stack);
    char *new_str5 = ThreadedStackPop(new_stack);

    assert_int_equal(strcmp(new_str1, "5"), 0);
    assert_int_equal(strcmp(new_str2, "4"), 0);
    assert_int_equal(strcmp(new_str3, "3"), 0);
    assert_int_equal(strcmp(new_str4, "2"), 0);
    assert_int_equal(strcmp(new_str5, "1"), 0);

    free(new_str1);
    free(new_str2);
    free(new_str3);
    free(new_str4);
    free(new_str5);

    ThreadedStackDestroy(new_stack);
}

static void test_push_report_count(void)
{
    ThreadedStack *stack = ThreadedStackNew(0, free);

    size_t size1 = ThreadedStackPushReportCount(stack, xstrdup("1"));
    size_t size2 = ThreadedStackPushReportCount(stack, xstrdup("2"));
    size_t size3 = ThreadedStackPushReportCount(stack, xstrdup("3"));
    size_t size4 = ThreadedStackPushReportCount(stack, xstrdup("4"));

    assert_int_equal(size1, 1);
    assert_int_equal(size2, 2);
    assert_int_equal(size3, 3);
    assert_int_equal(size4, 4);

    ThreadedStackDestroy(stack);
}

static void test_expand(void)
{
    ThreadedStack *stack = ThreadedStackNew(1, free);

    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));
    ThreadedStackPush(stack, xstrdup("spam"));

    assert_int_equal(ThreadedStackCount(stack), 9);
    assert_int_equal(ThreadedStackCapacity(stack), 16);

    ThreadedStackDestroy(stack);
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_push_pop),
        unit_test(test_pop_empty_and_push_null),
        unit_test(test_copy),
        unit_test(test_push_report_count),
        unit_test(test_expand),
    };
    return run_tests(tests);
}
