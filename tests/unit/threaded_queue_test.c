#include <test.h>

#include <alloc.h>
#include <mutex.h>
#include <threaded_queue.h>

/* Memory illustration legend:          *
 *      | : memory bounds               *
 *      > : head                        *
 *      < : tail                        *
 *      ^ : head + tail (empty)         *
 *      v : head + tail (at capacity)   *
 *      x : used memory                 *
 *      - : unused memory               */

static void test_push_pop(void)
{
    // Initialised with DEFAULT_CAPACITY = 16
    ThreadedQueue *queue = ThreadedQueueNew(0, free);
    // |^---------------|
    ThreadedQueuePush(queue, xstrdup("1"));
    // |><--------------|
    ThreadedQueuePush(queue, xstrdup("2"));
    // |>x<-------------|
    ThreadedQueuePush(queue, xstrdup("3"));
    // |>xx<------------|

    char *str1; ThreadedQueuePop(queue, (void **)&str1, 0);
    // |->x<------------|
    char *str2; ThreadedQueuePop(queue, (void **)&str2, 0);
    // |--><------------|
    char *str3; ThreadedQueuePop(queue, (void **)&str3, 0);
    // |---v------------|

    assert_string_equal(str1, "1");
    assert_string_equal(str2, "2");
    assert_string_equal(str3, "3");

    free(str1);
    free(str2);
    free(str3);

    ThreadedQueueDestroy(queue);
}

static void test_pop_empty_and_push_null(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(1, NULL);
    // |^|

    assert(ThreadedQueueIsEmpty(queue));

    void *i_am_null = NULL; bool ret = ThreadedQueuePop(queue, &i_am_null, 0);
    // |^|
    assert(i_am_null == NULL);
    assert_false(ret);
    ThreadedQueuePush(queue, i_am_null);
    // |v|
    ret = ThreadedQueuePop(queue, &i_am_null, 0);
    assert(i_am_null == NULL);
    assert_true(ret);
    // |^|

    ThreadedQueueDestroy(queue);
}

static void test_copy(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(4, free);
    // queue: |^---|

    ThreadedQueuePush(queue, xstrdup("1"));
    // queue: |><--|
    ThreadedQueuePush(queue, xstrdup("2"));
    // queue: |>x<-|
    ThreadedQueuePush(queue, xstrdup("3"));
    // queue: |>xx<|

    ThreadedQueue *new_queue = ThreadedQueueCopy(queue);
    // new_queue: |>xx<|

    assert(new_queue != NULL);
    assert_int_equal(ThreadedQueueCount(queue),
                     ThreadedQueueCount(new_queue));
    assert_int_equal(ThreadedQueueCapacity(queue),
                     ThreadedQueueCapacity(new_queue));

    char *old_str1; ThreadedQueuePop(queue, (void **)&old_str1, 0);
    // queue: |->x<|
    char *old_str2; ThreadedQueuePop(queue, (void **)&old_str2, 0);
    // queue: |--><|
    char *old_str3; ThreadedQueuePop(queue, (void **)&old_str3, 0);
    // queue: |---^|

    char *new_str1; ThreadedQueuePop(new_queue, (void **)&new_str1, 0);
    // new_queue: |->x<|
    char *new_str2; ThreadedQueuePop(new_queue, (void **)&new_str2, 0);
    // new_queue: |--><|
    char *new_str3; ThreadedQueuePop(new_queue, (void **)&new_str3, 0);
    // new_queue: |---^|

    // Check if pointers are equal (since this is a shallow copy)
    assert(old_str1 == new_str1);
    assert(old_str2 == new_str2);
    assert(old_str3 == new_str3);

    free(old_str1);
    free(old_str2);
    free(old_str3);

    ThreadedQueueSoftDestroy(queue);

    // Tests expanding the copied queue
    ThreadedQueuePush(new_queue, xstrdup("1"));
    // Internal array wraps:
    // new_queue: |<-->|
    ThreadedQueuePush(new_queue, xstrdup("2"));
    // new_queue: |x<->|
    ThreadedQueuePush(new_queue, xstrdup("3"));
    // new_queue: |xx<>|
    ThreadedQueuePush(new_queue, xstrdup("4"));
    // new_queue: |xxxv|
    ThreadedQueuePush(new_queue, xstrdup("5"));
    // Internal array restructured, array moved to end:
    // new_queue: |<-->xxxx|

    assert_int_equal(ThreadedQueueCount(new_queue), 5);
    assert_int_equal(ThreadedQueueCapacity(new_queue), 8);

    ThreadedQueuePop(new_queue, (void **)&new_str1, 0);
    // new_queue: |<--->xxx|
    ThreadedQueuePop(new_queue, (void **)&new_str2, 0);
    // new_queue: |<---->xx|
    ThreadedQueuePop(new_queue, (void **)&new_str3, 0);
    // new_queue: |<----->x|
    char *new_str4; ThreadedQueuePop(new_queue, (void **)&new_str4, 0);
    // new_queue: |<------>|
    char *new_str5; ThreadedQueuePop(new_queue, (void **)&new_str5, 0);
    // new_queue: |^-------|

    assert_string_equal(new_str1, "1");
    assert_string_equal(new_str2, "2");
    assert_string_equal(new_str3, "3");
    assert_string_equal(new_str4, "4");
    assert_string_equal(new_str5, "5");

    free(new_str1);
    free(new_str2);
    free(new_str3);
    free(new_str4);
    free(new_str5);

    ThreadedQueueDestroy(new_queue);
}

static void test_push_report_count(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, free);

    size_t size1 = ThreadedQueuePush(queue, xstrdup("1"));
    size_t size2 = ThreadedQueuePush(queue, xstrdup("2"));
    size_t size3 = ThreadedQueuePush(queue, xstrdup("3"));
    size_t size4 = ThreadedQueuePush(queue, xstrdup("4"));

    assert_int_equal(size1, 1);
    assert_int_equal(size2, 2);
    assert_int_equal(size3, 3);
    assert_int_equal(size4, 4);

    ThreadedQueueDestroy(queue);
}

static void test_expand(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(1, free);
    // |^|

    ThreadedQueuePush(queue, xstrdup("spam"));
    // |v|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |vx|

    char *tmp; ThreadedQueuePop(queue, (void **)&tmp, 0);
    // |<>|
    free(tmp);

    ThreadedQueuePush(queue, xstrdup("spam"));
    // |xv|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // Internal array restructured:
    // |<>xx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |xvxx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // Internal array restructured:
    // |->xxxx<-|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |->xxxxx<|

    ThreadedQueuePop(queue, (void **)&tmp, 0);
    // |-->xxxx<|
    free(tmp);
    ThreadedQueuePop(queue, (void **)&tmp, 0);
    // |--->xxx<|
    free(tmp);

    ThreadedQueuePush(queue, xstrdup("spam"));
    // |<-->xxxx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |x<->xxxx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |xx<>xxxx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |xxxvxxxx|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // Internal array restructured
    // |--->xxxxxxxx<---|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |--->xxxxxxxxx<--|
    ThreadedQueuePush(queue, xstrdup("spam"));
    // |--->xxxxxxxxxx<-|

    assert_int_equal(ThreadedQueueCount(queue), 11);
    assert_int_equal(ThreadedQueueCapacity(queue), 16);

    ThreadedQueueDestroy(queue);
}

static void test_pushn(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, NULL);

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};
    size_t count = ThreadedQueuePushN(queue, (void**) strs, 5);
    assert_int_equal(count, 5);
    count = ThreadedQueueCount(queue);
    assert_int_equal(count, 5);

    for (int i = 0; i < 5; i++)
    {
        char *item;
        ThreadedQueuePop(queue, (void **)&item, 0);
        assert_string_equal(item, strs[i]);
    }
    count = ThreadedQueueCount(queue);
    assert_int_equal(count, 0);

    ThreadedQueueDestroy(queue);
}

static void test_popn(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, free);
    // Initialised with default size 16
    // |^---------------|

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};

    for (int i = 0; i < 5; i++)
    {
        ThreadedQueuePush(queue, xstrdup(strs[i]));
    }
    // |>xxxx<----------|

    void **data = NULL;
    size_t count = ThreadedQueuePopN(queue, &data, 5, 0);
    // |-----^----------|

    for (size_t i = 0; i < count; i++)
    {
        assert_string_equal(data[i], strs[i]);
        free(data[i]);
    }

    free(data);
    ThreadedQueueDestroy(queue);
}

static void test_popn_into_array(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, free);
    // Initialised with default size 16
    // |^---------------|

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};

    for (int i = 0; i < 5; i++)
    {
        ThreadedQueuePush(queue, xstrdup(strs[i]));
    }
    // |>xxxx<----------|

    void *data[7] = {NULL, NULL, NULL, NULL, NULL, "test1", "test2"};
    size_t count = ThreadedQueuePopNIntoArray(queue, data, 5, 0);
    // |-----^----------|

    assert_int_equal(count, 5);
    for (size_t i = 0; i < count; i++)
    {
        assert_string_equal(data[i], strs[i]);
        free(data[i]);
    }
    assert_string_equal(data[5], "test1");
    assert_string_equal(data[6], "test2");

    ThreadedQueueDestroy(queue);
}

static void test_clear(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, free);

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};

    for (int i = 0; i < 5; i++)
    {
        ThreadedQueuePush(queue, xstrdup(strs[i]));
    }
    size_t count = ThreadedQueueCount(queue);
    assert_int_equal(count, 5);

    ThreadedQueueClear(queue);
    count = ThreadedQueueCount(queue);
    assert_int_equal(count, 0);

    ThreadedQueuePush(queue, xstrdup(strs[4]));
    char *item;
    ThreadedQueuePop(queue, (void **) &item, THREAD_BLOCK_INDEFINITELY);
    assert_string_equal(item, strs[4]);
    free(item);

    ThreadedQueueDestroy(queue);
}

static void test_clear_and_push(void)
{
    ThreadedQueue *queue = ThreadedQueueNew(0, NULL);

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};

    for (int i = 0; i < 4; i++)
    {
        ThreadedQueuePush(queue, strs[i]);
    }
    size_t count = ThreadedQueueCount(queue);
    assert_int_equal(count, 4);

    count = ThreadedQueueClearAndPush(queue, strs[4]);
    assert_int_equal(count, 1);
    count = ThreadedQueueCount(queue);
    assert_int_equal(count, 1);

    char *item;
    ThreadedQueuePop(queue, (void **)&item, 0);
    assert_string_equal(item, strs[4]);

    ThreadedQueueDestroy(queue);
}

// Thread tests
static ThreadedQueue *thread_queue;

static void *thread_pop()
{
    char *tmp;
    ThreadedQueuePop(thread_queue, (void **)&tmp, THREAD_BLOCK_INDEFINITELY);
    assert_string_equal(tmp, "bla");
    free(tmp);

    return NULL;
}

/**
 * Used in test_threads_pushn(). Tries to pop 5 items while there should only be
 * 3 which are all pushed at once. So the attempt should always pop 3 items.
 */
static void *thread_pop_5_3()
{
    char **items;
    size_t n_popped = ThreadedQueuePopN(thread_queue, (void***)&items, 5, THREAD_BLOCK_INDEFINITELY);
    assert_int_equal(n_popped, 3);
    free(items);

    return NULL;
}

static void *thread_push()
{
    char *str = "bla";
    ThreadedQueuePush(thread_queue, xstrdup(str));

    return NULL;
}

static void *thread_wait_empty()
{
    ThreadedQueueWaitEmpty(thread_queue, THREAD_BLOCK_INDEFINITELY);
    ThreadedQueuePush(thread_queue, xstrdup("a_test"));

    return NULL;
}

/* Used in the test_threads_clear_empty */
static void *thread_just_wait_empty()
{
    ThreadedQueueWaitEmpty(thread_queue, THREAD_BLOCK_INDEFINITELY);

    return NULL;
}

static void test_threads_wait_pop(void)
{
#define POP_ITERATIONS 100
    thread_queue = ThreadedQueueNew(0, free);

    pthread_t pops[POP_ITERATIONS] = {0};
    for (int i = 0; i < POP_ITERATIONS; i++)
    {
        int res_create = pthread_create(&(pops[i]), NULL,
                                        thread_pop, NULL);
        assert_int_equal(res_create, 0);
    }

    pthread_t pushs[POP_ITERATIONS] = {0};
    for (int i = 0; i < POP_ITERATIONS; i++)
    {
        int res_create = pthread_create(&(pushs[i]), NULL,
                                        thread_push, NULL);
        assert_int_equal(res_create, 0);
    }

    void *retval = NULL;
    int res;

    for (int i = 0; i < POP_ITERATIONS; i++)
    {
        res = pthread_join(pops[i], retval);
        assert_int_equal(res, 0);
        assert(retval == NULL);

        res = pthread_join(pushs[i], retval);
        assert_int_equal(res, 0);
        assert(retval == NULL);
    }

    ThreadedQueueDestroy(thread_queue);
}

static void test_threads_wait_empty(void)
{
#define WAIT_ITERATIONS 100
    thread_queue = ThreadedQueueNew(0, free);

    pthread_t pushs[WAIT_ITERATIONS] = {0};
    for (int i = 0; i < WAIT_ITERATIONS; i++)
    {
        int res_create = pthread_create(&(pushs[i]), NULL,
                                        thread_push, NULL);
        assert_int_equal(res_create, 0);
    }

    sleep(1);
    pthread_t wait_thread = 0;
    int res_create = pthread_create(&wait_thread, NULL,
                                    thread_wait_empty, NULL);
    assert_int_equal(res_create, 0);

    do {
        sleep(1);
    } while (ThreadedQueueCount(thread_queue) != WAIT_ITERATIONS);

    char **data_array = NULL;
    size_t arr_size = ThreadedQueuePopN(thread_queue, (void ***)&data_array,
                                        WAIT_ITERATIONS, 0);

    for (size_t i = 0; i < arr_size; i++)
    {
        free(data_array[i]);
    }

    free(data_array);

    char *waited_str; ThreadedQueuePop(thread_queue, (void **)&waited_str, 1);
    assert_string_equal(waited_str, "a_test");
    free(waited_str);

    void *retval = NULL;
    int res;

    for (int i = 0; i < WAIT_ITERATIONS; i++)
    {
        res = pthread_join(pushs[i], retval);
        assert_int_equal(res, 0);
        assert(retval == NULL);
    }

    res = pthread_join(wait_thread, retval);
    assert_int_equal(res, 0);
    assert(retval == NULL);

    ThreadedQueueDestroy(thread_queue);
}

static void test_threads_pushn()
{
    thread_queue = ThreadedQueueNew(0, NULL);

    pthread_t pop_thread;
    int res = pthread_create(&pop_thread, NULL,
                             thread_pop_5_3, NULL);
    assert_int_equal(res, 0);

    /* give the other thread time to start waiting */
    sleep(1);

    char *strs[] = {"spam1", "spam2", "spam3"};
    size_t count = ThreadedQueuePushN(thread_queue, (void **)strs, 3);
    assert_int_equal(count, 3);

    res = pthread_join(pop_thread, NULL);
    assert_int_equal(res, 0);

    count = ThreadedQueueCount(thread_queue);
    assert_int_equal(count, 0);

    ThreadedQueueDestroy(thread_queue);
}

static void test_threads_clear_empty()
{
    thread_queue = ThreadedQueueNew(0, NULL);

    char *strs[] = {"spam1", "spam2", "spam3", "spam4", "spam5"};

    for (int i = 0; i < 5; i++)
    {
        ThreadedQueuePush(thread_queue, strs[i]);
    }
    size_t count = ThreadedQueueCount(thread_queue);
    assert_int_equal(count, 5);

    pthread_t wait_thread;
    int res = pthread_create(&wait_thread, NULL,
                             thread_just_wait_empty, NULL);
    assert_int_equal(res, 0);

    ThreadedQueueClear(thread_queue);
    count = ThreadedQueueCount(thread_queue);
    assert_int_equal(count, 0);

    res = pthread_join(wait_thread, NULL);
    assert_int_equal(res, 0);

    ThreadedQueueDestroy(thread_queue);
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
        unit_test(test_popn),
        unit_test(test_popn_into_array),
        unit_test(test_pushn),
        unit_test(test_clear),
        unit_test(test_clear_and_push),
        unit_test(test_threads_wait_pop),
        unit_test(test_threads_wait_empty),
        unit_test(test_threads_pushn),
        unit_test(test_threads_clear_empty),
    };
    return run_tests(tests);
}
