#include <test.h>

#include <sequence.c>
#include <string_sequence.c>
#include <alloc.h>

static Seq *SequenceCreateRange(size_t initialCapacity, size_t start, size_t end)
{
    Seq *seq = SeqNew(initialCapacity, free);

    for (size_t i = start; i <= end; i++)
    {
        size_t *item = xmalloc(sizeof(size_t));

        *item = i;
        SeqAppend(seq, item);
    }

    return seq;
}

static void test_create_destroy(void)
{
    Seq *seq = SeqNew(5, NULL);

    SeqDestroy(seq);
}

static void test_append(void)
{
    Seq *seq = SeqNew(2, free);

    for (size_t i = 0; i < 1000; i++)
    {
        SeqAppend(seq, xstrdup("snookie"));
    }

    assert_int_equal(seq->length, 1000);

    for (size_t i = 0; i < 1000; i++)
    {
        assert_string_equal(seq->data[i], "snookie");
    }

    SeqDestroy(seq);
}

static void test_set(void)
{
    Seq *seq = SeqNew(10, free);
    for (size_t i = 0; i < 10; i++)
    {
        SeqAppend(seq, xstrdup("snookie"));
    }
    assert_int_equal(seq->length, 10);

    SeqSet(seq, 0, xstrdup("blah"));
    assert_int_equal(seq->length, 10);
    assert_string_equal(SeqAt(seq, 0), "blah");

    char *item = SeqAt(seq, 5);
    assert_string_equal(item, "snookie");
    SeqSoftSet(seq, 5, xstrdup("blah"));
    assert_int_equal(seq->length, 10);
    assert_string_equal(SeqAt(seq, 5), "blah");

    /* SeqSoftSet() shouldn't have destroyed the item so we should (be able to)
     * free it and SeqDestroy() should destroy the replacement. */
    free(item);
    SeqDestroy(seq);
}

static int CompareNumbers(const void *a, const void *b,
                          ARG_UNUSED void *_user_data)
{
    return *(size_t *) a - *(size_t *) b;
}

static void test_append_once(void)
{
    Seq *seq = SequenceCreateRange(10, 0, 9);

    for (size_t i = 0; i <= 9; i++)
    {
        size_t *item = xmalloc(sizeof(size_t));

        *item = i;
        SeqAppendOnce(seq, item, CompareNumbers);
    }

    /* none of the numbers above should have been inserted second time */
    assert_int_equal(seq->length, 10);

    size_t *item = xmalloc(sizeof(size_t));

    *item = 10;
    SeqAppendOnce(seq, item, CompareNumbers);

    /* 10 should have been inserted (as the first instance) */
    assert_int_equal(seq->length, 11);
    SeqDestroy(seq);
}

static void test_lookup(void)
{
    Seq *seq = SequenceCreateRange(10, 0, 9);

    size_t *key = xmalloc(sizeof(size_t));

    *key = 5;

    size_t *result = SeqLookup(seq, key, CompareNumbers);
    assert_int_equal(*result, *key);

    *key = 17;
    result = SeqLookup(seq, key, CompareNumbers);
    assert_int_equal(result, NULL);

    SeqDestroy(seq);
    free(key);
}

static void test_binary_lookup(void)
{
    size_t *key = xmalloc(sizeof(size_t));
    size_t *result;

    // Even numbered length.
    Seq *seq = SequenceCreateRange(10, 0, 9);
    for (size_t i = 0; i <= 9; i++)
    {
        *key = i;
        result = SeqBinaryLookup(seq, key, CompareNumbers);
        assert_int_equal(*result, *key);
    }
    *key = 17;
    result = SeqBinaryLookup(seq, key, CompareNumbers);
    assert_int_equal(result, NULL);

    // Odd numbered length.
    SeqDestroy(seq);
    seq = SequenceCreateRange(10, 0, 10);
    for (size_t i = 0; i <= 10; i++)
    {
        *key = i;
        result = SeqBinaryLookup(seq, key, CompareNumbers);
        assert_int_equal(*result, *key);
    }
    *key = 17;
    result = SeqBinaryLookup(seq, key, CompareNumbers);
    assert_int_equal(result, NULL);

    // Zero-length.
    SeqDestroy(seq);
    seq = SeqNew(0, free);
    *key = 0;
    result = SeqBinaryLookup(seq, key, CompareNumbers);
    assert_int_equal(result, NULL);

    SeqDestroy(seq);
    free(key);
}

static void test_index_of(void)
{
    Seq *seq = SequenceCreateRange(10, 0, 9);

    size_t *key = xmalloc(sizeof(size_t));

    *key = 5;

    ssize_t index = SeqIndexOf(seq, key, CompareNumbers);
    assert_int_equal(index, 5);

    *key = 17;
    index = SeqIndexOf(seq, key, CompareNumbers);
    assert_true(index == -1);

    SeqDestroy(seq);
    free(key);
}

static void test_binary_index_of(void)
{
    size_t *key = xmalloc(sizeof(size_t));
    ssize_t result;

    // Even numbered length.
    Seq *seq = SequenceCreateRange(10, 0, 9);
    for (size_t i = 0; i <= 9; i++)
    {
        *key = i;
        result = SeqBinaryIndexOf(seq, key, CompareNumbers);
        assert_int_equal(result, i);
    }
    *key = 17;
    result = SeqBinaryIndexOf(seq, key, CompareNumbers);
    assert_true(result == -1);

    // Odd numbered length.
    SeqDestroy(seq);
    seq = SequenceCreateRange(10, 0, 10);
    for (size_t i = 0; i <= 10; i++)
    {
        *key = i;
        result = SeqBinaryIndexOf(seq, key, CompareNumbers);
        assert_int_equal(result, i);
    }
    *key = 17;
    result = SeqBinaryIndexOf(seq, key, CompareNumbers);
    assert_true(result == -1);

    // Zero-length.
    SeqDestroy(seq);
    seq = SeqNew(0, free);
    *key = 0;
    result = SeqBinaryIndexOf(seq, key, CompareNumbers);
    assert_true(result == -1);

    SeqDestroy(seq);
    seq = SeqNew(5, free);
    SeqAppend(seq, xmalloc(sizeof(size_t))); *(size_t *)SeqAt(seq, 0) = 3;
    SeqAppend(seq, xmalloc(sizeof(size_t))); *(size_t *)SeqAt(seq, 1) = 3;
    SeqAppend(seq, xmalloc(sizeof(size_t))); *(size_t *)SeqAt(seq, 2) = 3;
    SeqAppend(seq, xmalloc(sizeof(size_t))); *(size_t *)SeqAt(seq, 3) = 3;
    SeqAppend(seq, xmalloc(sizeof(size_t))); *(size_t *)SeqAt(seq, 4) = 3;
    *key = 3;
    result = SeqBinaryIndexOf(seq, key, CompareNumbers);
    // Any number within the range is ok.
    assert_true(result >= 0 && result < 5);

    SeqDestroy(seq);
    free(key);
}

static void test_sort(void)
{
    Seq *seq = SeqNew(5, NULL);

    size_t one = 1;
    size_t two = 2;
    size_t three = 3;
    size_t four = 4;
    size_t five = 5;

    SeqAppend(seq, &three);
    SeqAppend(seq, &two);
    SeqAppend(seq, &five);
    SeqAppend(seq, &one);
    SeqAppend(seq, &four);

    SeqSort(seq, CompareNumbers, NULL);

    assert_int_equal(seq->data[0], &one);
    assert_int_equal(seq->data[1], &two);
    assert_int_equal(seq->data[2], &three);
    assert_int_equal(seq->data[3], &four);
    assert_int_equal(seq->data[4], &five);

    SeqDestroy(seq);
}

static void test_soft_sort(void)
{
    Seq *seq = SeqNew(5, NULL);

    size_t one = 1;
    size_t two = 2;
    size_t three = 3;
    size_t four = 4;
    size_t five = 5;

    SeqAppend(seq, &three);
    SeqAppend(seq, &two);
    SeqAppend(seq, &five);
    SeqAppend(seq, &one);
    SeqAppend(seq, &four);

    Seq *new_seq = SeqSoftSort(seq, CompareNumbers, NULL);

    assert_int_equal(seq->data[0], &three);
    assert_int_equal(seq->data[1], &two);
    assert_int_equal(seq->data[2], &five);
    assert_int_equal(seq->data[3], &one);
    assert_int_equal(seq->data[4], &four);

    assert_int_equal(new_seq->data[0], &one);
    assert_int_equal(new_seq->data[1], &two);
    assert_int_equal(new_seq->data[2], &three);
    assert_int_equal(new_seq->data[3], &four);
    assert_int_equal(new_seq->data[4], &five);

    // This is a soft destroy, but normal destroy should also work.
    SeqDestroy(new_seq);

    SeqDestroy(seq);
}

static void test_remove_range(void)
{

    Seq *seq = SequenceCreateRange(10, 0, 9);

    SeqRemoveRange(seq, 3, 9);
    assert_int_equal(seq->length, 3);
    assert_int_equal(*(size_t *) seq->data[0], 0);
    assert_int_equal(*(size_t *) seq->data[1], 1);
    assert_int_equal(*(size_t *) seq->data[2], 2);

    SeqDestroy(seq);
    seq = SequenceCreateRange(10, 0, 9);

    SeqRemoveRange(seq, 0, 2);
    assert_int_equal(seq->length, 7);
    assert_int_equal(*(size_t *) seq->data[0], 3);

    SeqDestroy(seq);

    seq = SequenceCreateRange(10, 0, 9);

    SeqRemoveRange(seq, 5, 5);
    assert_int_equal(seq->length, 9);
    assert_int_equal(*(size_t *) seq->data[5], 6);

    SeqDestroy(seq);
}

static void test_remove(void)
{

    Seq *seq = SequenceCreateRange(10, 0, 9);

    SeqRemove(seq, 5);

    assert_int_equal(seq->length, 9);
    assert_int_equal(*(size_t *) seq->data[5], 6);

    SeqDestroy(seq);
    seq = SequenceCreateRange(10, 0, 9);

    SeqRemove(seq, 0);
    assert_int_equal(seq->length, 9);
    assert_int_equal(*(size_t *) seq->data[0], 1);

    SeqDestroy(seq);

    seq = SequenceCreateRange(10, 0, 9);

    SeqRemove(seq, 9);
    assert_int_equal(seq->length, 9);
    assert_int_equal(*(size_t *) seq->data[8], 8);

    SeqDestroy(seq);
}

static void test_reverse(void)
{
    {
        Seq *seq = SequenceCreateRange(2, 0, 1);
        assert_int_equal(0, *(size_t *)seq->data[0]);
        assert_int_equal(1, *(size_t *)seq->data[1]);
        SeqReverse(seq);
        assert_int_equal(1, *(size_t *)seq->data[0]);
        assert_int_equal(0, *(size_t *)seq->data[1]);
        SeqDestroy(seq);
    }

    {
        Seq *seq = SequenceCreateRange(3, 0, 2);
        SeqReverse(seq);
        assert_int_equal(2, *(size_t *)seq->data[0]);
        assert_int_equal(1, *(size_t *)seq->data[1]);
        assert_int_equal(0, *(size_t *)seq->data[2]);
        SeqDestroy(seq);
    }
}

static void test_split(void)
{
    {
        Seq *seq = SeqNew(0, free);
        SeqAppend(seq, xstrdup("abc"));
        SeqAppend(seq, xstrdup("def"));
        assert_int_equal(SeqLength(seq), 2);

        Seq *end = SeqSplit(seq, 1);

        assert_int_equal(SeqLength(seq), 1);
        assert_int_equal(SeqLength(end), 1);

        assert_string_equal(SeqAt(seq, 0), "abc");
        assert_string_equal(SeqAt(end, 0), "def");

        SeqDestroy(seq);
        SeqDestroy(end);
    }

    {
        Seq *seq = SeqNew(0, free);
        SeqAppend(seq, xstrdup("abc"));
        SeqAppend(seq, xstrdup("def"));
        SeqAppend(seq, xstrdup("ghi"));
        SeqAppend(seq, xstrdup("jkl"));
        assert_int_equal(SeqLength(seq), 4);

        Seq *end = SeqSplit(seq, 2);

        assert_int_equal(SeqLength(seq), 2);
        assert_int_equal(SeqLength(end), 2);

        assert_string_equal(SeqAt(seq, 0), "abc");
        assert_string_equal(SeqAt(seq, 1), "def");
        assert_string_equal(SeqAt(end, 0), "ghi");
        assert_string_equal(SeqAt(end, 1), "jkl");

        SeqDestroy(seq);
        SeqDestroy(end);
    }

    {
        Seq *empty_a = SeqNew(0, free);
        assert_int_equal(SeqLength(empty_a), 0);

        Seq *empty_b = SeqSplit(empty_a, 0);

        assert_int_equal(SeqLength(empty_a), 0);
        assert_int_equal(SeqLength(empty_b), 0);

        SeqDestroy(empty_a);
        SeqDestroy(empty_b);
    }

    {
        Seq *seq = SeqNew(0, free);
        SeqAppend(seq, xstrdup("abc"));
        assert_int_equal(SeqLength(seq), 1);

        Seq *empty = SeqSplit(seq, 1);

        assert_int_equal(SeqLength(seq), 1);
        assert_int_equal(SeqLength(empty), 0);

        assert_string_equal(SeqAt(seq, 0), "abc");

        SeqDestroy(seq);
        SeqDestroy(empty);
    }

    {
        Seq *seq = SeqNew(0, free);
        SeqAppend(seq, xstrdup("abc"));
        assert_int_equal(SeqLength(seq), 1);

        Seq *all = SeqSplit(seq, 0);

        assert_int_equal(SeqLength(seq), 0);
        assert_int_equal(SeqLength(all), 1);

        assert_string_equal(SeqAt(all, 0), "abc");

        SeqDestroy(seq);
        SeqDestroy(all);
    }
}

static void test_len(void)
{
    Seq *seq = SeqNew(5, NULL);

    size_t one = 1;
    size_t two = 2;
    size_t three = 3;
    size_t four = 4;
    size_t five = 5;

    SeqAppend(seq, &three);
    SeqAppend(seq, &two);
    SeqAppend(seq, &five);
    SeqAppend(seq, &one);
    SeqAppend(seq, &four);

    assert_int_equal(SeqLength(seq),5);

    SeqDestroy(seq);
}

static void test_get_range(void)
{
    Seq *seq = SeqNew(5, NULL);

    size_t one = 1;
    size_t two = 2;
    size_t three = 3;
    size_t four = 4;
    size_t five = 5;

    SeqAppend(seq, &three);
    SeqAppend(seq, &two);
    SeqAppend(seq, &five);
    SeqAppend(seq, &one);
    SeqAppend(seq, &four);

    assert_int_equal(SeqLength(seq),5);

    {
        Seq *sub_1 = SeqGetRange(seq, 0, 4);
        assert_true (sub_1 != NULL);
        assert_int_equal (sub_1->length, seq->length);
        assert_int_equal (SeqAt(sub_1, 0), SeqAt(seq, 0));
        assert_int_equal (SeqAt(sub_1, 1), SeqAt(seq, 1));
        assert_int_equal (SeqAt(sub_1, 2), SeqAt(seq, 2));
        assert_int_equal (SeqAt(sub_1, 3), SeqAt(seq, 3));
        assert_int_equal (SeqAt(sub_1, 4), SeqAt(seq, 4));
        SeqSoftDestroy(sub_1);
    }

    {
        Seq *sub_1 = SeqGetRange(seq, 2, 4);
        assert_true (sub_1 != NULL);
        assert_int_equal (sub_1->length, 4 - 2 + 1);
        assert_int_equal (SeqAt(sub_1, 0), SeqAt(seq, 2));
        assert_int_equal (SeqAt(sub_1, 1), SeqAt(seq, 3));
        assert_int_equal (SeqAt(sub_1, 2), SeqAt(seq, 4));
        SeqSoftDestroy(sub_1);
    }

    assert_true (!SeqGetRange(seq, 3, 6));
    assert_true (!SeqGetRange(seq, 3, 2));

    SeqDestroy(seq);
}

static void test_get_data(void)
{
    Seq *seq = SeqNew(16, NULL);
    SeqAppend(seq, "one");
    SeqAppend(seq, "two");
    SeqAppend(seq, "three");
    SeqAppend(seq, NULL);

    void *const *data = SeqGetData(seq);

    assert_string_equal(data[0], "one");
    assert_string_equal(data[1], "two");
    assert_string_equal(data[2], "three");
    assert_true(data[3] == NULL);

    /* Check that accessing the remaining pointers is OK */
    for (size_t i = 4; i < 16; i++)
    {
        void *item = data[i];
        UNUSED(item);
    }

    /* no leak here, 'data' is not a copy */
    SeqDestroy(seq);
}

static void test_seq_string_contains(void)
{
    Seq *strings = SeqNew(10, NULL);
    assert_false(SeqStringContains(strings, ""));
    assert_false(SeqStringContains(strings, "a"));
    assert_false(SeqStringContains(strings, " "));
    assert_false(SeqStringContains(strings, "nosuch"));

    SeqAppend(strings, "a");
    assert_false(SeqStringContains(strings, ""));
    assert_true(SeqStringContains(strings, "a"));
    assert_false(SeqStringContains(strings, " "));
    assert_false(SeqStringContains(strings, "nosuch"));

    SeqAppend(strings, " ");
    assert_false(SeqStringContains(strings, ""));
    assert_true(SeqStringContains(strings, "a"));
    assert_true(SeqStringContains(strings, " "));
    assert_false(SeqStringContains(strings, "nosuch"));

    SeqAppend(strings, "");
    assert_true(SeqStringContains(strings, ""));
    assert_true(SeqStringContains(strings, "a"));
    assert_true(SeqStringContains(strings, " "));
    assert_false(SeqStringContains(strings, "nosuch"));

    SeqAppend(strings, "abcdefghijklmnopqrstuvwxyz");
    assert_true(SeqStringContains(strings, "abcdefghijklmnopqrstuvwxyz"));
    assert_false(SeqStringContains(strings, "abcdefghijklmnopqrstuvwxyz "));
    assert_false(SeqStringContains(strings, "abcdefghijklmnopqrstuvwxy"));
    assert_false(SeqStringContains(strings, "abcdefghijklmnopqrstuvwxyy"));

    SeqDestroy(strings);
}

static void test_seq_string_length(void)
{
    Seq *strings = SeqNew(10, NULL);
    assert_int_equal(SeqStringLength(strings), 0);
    SeqAppend(strings, "1");
    assert_int_equal(SeqStringLength(strings), 1);
    SeqAppend(strings, "2345678");
    assert_int_equal(SeqStringLength(strings), 8);
    SeqAppend(strings, "");
    SeqAppend(strings, "9");
    SeqAppend(strings, "");
    assert_int_equal(SeqStringLength(strings), 9);
    SeqDestroy(strings);
}

static void test_string_deserialize(void)
{
    {
        char *two_newlines = xstrdup("1         \n\n1         \n\n");
        Seq *seq = SeqStringDeserialize(two_newlines);
        assert_true(seq != NULL);
        free(two_newlines); // Copies should be allocated
        const char *a = SeqAt(seq, 0);
        const char *b = SeqAt(seq, 1);
        assert_string_equal(a, "\n");
        assert_string_equal(b, "\n");
        assert_int_equal(SeqLength(seq), 2);
        SeqDestroy(seq);
    }
    {
        // Any invalid string should return NULL:
        assert_true(SeqStringDeserialize(" ") == NULL);
        assert_true(SeqStringDeserialize("1") == NULL);

        // Missing newline:
        assert_true(SeqStringDeserialize("1         A") == NULL);
        assert_true(SeqStringDeserialize("2         A\n") == NULL);
        assert_true(SeqStringDeserialize("1         A ") == NULL);

        // NUL byte wrong (length wrong):
        assert_true(SeqStringDeserialize("10000     AAAAAAAAAA\n") == NULL);
        assert_true(SeqStringDeserialize("0         A\n") == NULL);
    }
    {
        // Empty String -> Empty Seq:
        Seq *seq = SeqStringDeserialize("");
        assert(seq != NULL);
        assert(SeqLength(seq) == 0);
        SeqDestroy(seq);
    }
}

static void test_string_serialize(void)
{
    {
        Seq *seq = SeqNew(100, free);
        char *str = SeqStringSerialize(seq);
        assert(str != NULL && str[0] == '\0');
        free(str);
        SeqDestroy(seq);
    }
    const char *three = "3         ABC\n3         DEF\n3         GHI\n";
    {
        Seq *seq = SeqNew(100, free);
        SeqAppend(seq, xstrdup("ABC"));
        SeqAppend(seq, xstrdup("DEF"));
        SeqAppend(seq, xstrdup("GHI"));
        char *serialized = SeqStringSerialize(seq);
        assert_string_equal(serialized, three);
        Seq *seq2 = SeqStringDeserialize(serialized);
        free(serialized);
        assert_true(seq2 != NULL);
        const char *abc = SeqAt(seq2, 0);
        const char *def = SeqAt(seq2, 1);
        const char *ghi = SeqAt(seq2, 2);
        assert_string_equal(abc, "ABC");
        assert_string_equal(def, "DEF");
        assert_string_equal(ghi, "GHI");
        SeqDestroy(seq);
        SeqDestroy(seq2);
    }
}

static void test_seq_string_file(void)
{
    const char *const path = "test_file_string_sequence";

    Seq *const sequence = SeqNew(5, free);
    SeqAppend(sequence, xstrdup("ABC"));
    SeqAppend(sequence, xstrdup("DEF"));
    SeqAppend(sequence, xstrdup("123"));

    const size_t length = SeqLength(sequence);
    assert_int_equal(length, 3);

    const bool write_success = SeqStringWriteFile(sequence, path);
    assert_true(write_success);

    Seq *const read_sequence = SeqStringReadFile(path);
    assert_true(read_sequence != NULL);

    assert_int_equal(length, SeqLength(sequence));
    assert_int_equal(length, SeqLength(read_sequence));

    for(int i = 0; i < length; ++i)
    {
        assert_string_equal(SeqAt(sequence, i), SeqAt(read_sequence, i));
    }
    SeqDestroy(sequence);
    SeqDestroy(read_sequence);
    unlink(path);
    assert_true(SeqStringReadFile(path) == NULL);
}

static void test_seq_string_empty_file(void)
{
    const char *const path = "test_file_string_sequence";

    Seq *const sequence = SeqNew(5, free);
    const bool write_success = SeqStringWriteFile(sequence, path);
    assert_true(write_success);

    struct stat statbuf;

    stat(path, &statbuf);
    assert_int_equal(0, statbuf.st_size);

    Seq *const read_sequence = SeqStringReadFile(path);
    assert_true(read_sequence != NULL);

    assert_int_equal(0, SeqLength(sequence));
    assert_int_equal(0, SeqLength(read_sequence));

    SeqDestroy(sequence);
    SeqDestroy(read_sequence);
    unlink(path);

    assert_true(SeqStringReadFile(path) == NULL);
}

void test_sscanf(void)
{
    // NOTE: sscanf() on HPUX does not match %z %j %zu %ju etc.
    //       use %ld and %lu (signed and unsigned long) instead
    const char *three = "3         ABC\n3         DEF\n3         GHI\n";
    const char *eleven = "11        ABC\n";

    unsigned long length;
    long long_num;
    int ret;

    ret = sscanf(three, "%lu", &length);
    assert_int_equal(ret, 1);
    assert_int_equal(length, 3);

    ret = sscanf(three, "%5lu'", &length);
    assert_int_equal(ret, 1);
    assert_int_equal(length, 3);

    ret = sscanf(three, "%10lu'", &length);
    assert_int_equal(ret, 1);
    assert_int_equal(length, 3);

    ret = sscanf(eleven, "%10lu'", &length);
    assert_int_equal(ret, 1);
    assert_int_equal(length, 11);

    ret = sscanf(eleven, "%10ld", &long_num);
    assert_int_equal(ret, 1);
    assert_int_equal(long_num, 11);

    // Test max field width for %s:
    const char *const one_to_nine = "123456789";
    char a[4];
    char b[4];
    char c[3];
    char d[2];
    ret = sscanf(one_to_nine, "%3s%3s%2s%1s", a, b, c, d);
    assert_int_equal(ret, 4);
    assert_string_equal(a, "123");
    assert_string_equal(b, "456");
    assert_string_equal(c, "78");
    assert_string_equal(d, "9");

    // Test scanning shorter strings than max:
    const char *const with_spaces = "abc de f";
    ret = sscanf(with_spaces, "%3s %3s %2s", a, b, c);
    assert_int_equal(ret, 3);
    assert_string_equal(a, "abc");
    assert_string_equal(b, "de");
    assert_string_equal(c, "f");

    const char *const enforce_spaces = "tu wxyz A";
    #define FMT_SPACE "%*[ ]" // Matches exactly 1 space and discards it
    ret = sscanf(enforce_spaces, "%3s"FMT_SPACE"%3s"FMT_SPACE"%2s", a, b, c);
    #undef FMT_SPACE
    assert_int_equal(ret, 2);
    assert_string_equal(a, "tu");
    assert_string_equal(b, "wxy");
    assert_string_equal(c, "f");   // Untouched

    // Note that in this example, %s could match commas:
    const char *const partial_match = "tuv,wxyz,A";
    ret = sscanf(partial_match, "%3s,%3s,%2s", a, b, c);
    assert_int_equal(ret, 2);
    assert_string_equal(a, "tuv");
    assert_string_equal(b, "wxy");
    assert_string_equal(c, "f");   // Untouched

    // Don't allow commas in strings:
    const char *const strict_commas = "tu,wxyz,A";
    ret = sscanf(strict_commas, "%3[^ ,],%3[^ ,],%2[^ ,]", a, b, c);
    // Stopped because second column was too long strlen("wxyz") > 3
    assert_int_equal(ret, 2);
    assert_string_equal(a, "tu");
    assert_string_equal(b, "wxy");
    assert_string_equal(c, "f");   // Untouched

    // Spaces paces allowed in strings
    const char *const spaces_allowed = "a b,   , a, ";
    ret = sscanf(spaces_allowed, "%3[^,],%3[^,],%2[^,],%1[^,]", a, b, c, d);
    // Stopped because second column was too long strlen("wxyz") > 3
    assert_int_equal(ret, 4);
    assert_string_equal(a, "a b");
    assert_string_equal(b, "   ");
    assert_string_equal(c, " a");
    assert_string_equal(d, " ");

    // Don't use sscanf to parse CSV rows:
    // 1. %[] cannot match empty string
    // 2. No way to handle quoting
}

void test_string_prefix(void)
{
    const char *three = "3         ABC\n3         DEF\n3         GHI\n";
    const char *eleven = "11        ABC\n";

    assert_int_equal(3,    GetLengthPrefix(three));
    assert_int_equal(11,   GetLengthPrefix(eleven));
    assert_int_equal(1234, GetLengthPrefix("1234      H\n"));
    assert_true(three[STR_LENGTH_PREFIX_LEN] == 'A');
}

void dupl_checker(const char *str)
{
    const size_t len = strlen(str);
    char *res = ValidDuplicate(str, len);
    assert_true(res != NULL);
    assert_true(res != str);
    assert_string_equal(res, str);
    free(res);
    for (long l = len + 1; l <= 2*len; ++l)
    {
        // String shorter than specified (expected) length:
        res = ValidDuplicate(str, l);
        assert_true(res == NULL);
    }
}

void test_valid_duplicate(void)
{
    dupl_checker("");
    dupl_checker("A");
    dupl_checker("AB");
    dupl_checker("ABC");
    dupl_checker("ABCD");
    dupl_checker("ABCDE");
    dupl_checker("ABCDEF");
    dupl_checker("ABCDEFG");
    dupl_checker(" ");
    dupl_checker("  ");
    dupl_checker("   ");
    dupl_checker("    ");
    dupl_checker("     ");
    dupl_checker("      ");
    dupl_checker("       ");
    dupl_checker("        ");
    dupl_checker("         ");
    dupl_checker("\n");
    dupl_checker(" \n");
    dupl_checker("\n ");
    dupl_checker("\r\n");
    dupl_checker("\n\r");
    dupl_checker(" \n ");
    dupl_checker(" \r\n ");
    dupl_checker(" \n\r ");
    dupl_checker("Lorem ipsum dolor sit amet.\nHello, world!\n\n");
}

int main()
{
    PRINT_TEST_BANNER();
    const UnitTest tests[] =
    {
        unit_test(test_create_destroy),
        unit_test(test_append),
        unit_test(test_set),
        unit_test(test_append_once),
        unit_test(test_lookup),
        unit_test(test_binary_lookup),
        unit_test(test_index_of),
        unit_test(test_binary_index_of),
        unit_test(test_sort),
        unit_test(test_soft_sort),
        unit_test(test_remove_range),
        unit_test(test_remove),
        unit_test(test_reverse),
        unit_test(test_split),
        unit_test(test_len),
        unit_test(test_get_range),
        unit_test(test_get_data),
        unit_test(test_sscanf),
        unit_test(test_seq_string_contains),
        unit_test(test_seq_string_length),
        unit_test(test_string_prefix),
        unit_test(test_valid_duplicate),
        unit_test(test_string_deserialize),
        unit_test(test_string_serialize),
        unit_test(test_seq_string_file),
        unit_test(test_seq_string_empty_file),
    };

    return run_tests(tests);
}
