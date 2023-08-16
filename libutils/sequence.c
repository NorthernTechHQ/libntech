/*
  Copyright 2023 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <platform.h>
#include <sequence.h>
#include <alloc.h>

static const size_t EXPAND_FACTOR = 2;

Seq *SeqNew(size_t initialCapacity, void (ItemDestroy) (void *item))
{
    Seq *seq = xmalloc(sizeof(Seq));

    if (initialCapacity <= 0)
    {
        initialCapacity = 1;
    }

    seq->capacity = initialCapacity;
    seq->length = 0;
    seq->data = xcalloc(sizeof(void *), initialCapacity);
    seq->ItemDestroy = ItemDestroy;

    return seq;
}

static void DestroyRange(Seq *seq, size_t start, size_t end)
{
    assert(seq != NULL);
    if (seq->ItemDestroy)
    {
        for (size_t i = start; i <= end; i++)
        {
            seq->ItemDestroy(seq->data[i]);
        }
    }
}

void SeqDestroy(Seq *seq)
{
    if (seq != NULL)
    {
        if (seq->length > 0)
        {
            DestroyRange(seq, 0, seq->length - 1);
        }
        SeqSoftDestroy(seq);
    }
}

void SeqSoftDestroy(Seq *seq)
{
    if (seq != NULL)
    {
        free(seq->data);
        free(seq);
    }
}

static void ExpandIfNeccessary(Seq *seq)
{
    assert(seq != NULL);
    assert(seq->length <= seq->capacity);

    if (seq->length == seq->capacity)
    {
        seq->capacity *= EXPAND_FACTOR;
        seq->data = xrealloc(seq->data, sizeof(void *) * seq->capacity);
    }
}

int StrCmpWrapper(const void *s1, const void *s2, void *user_data)
{
    UNUSED(user_data);
    return strcmp(s1, s2);
}

void SeqSet(Seq *seq, size_t index, void *item)
{
    assert(seq != NULL);
    assert(index < SeqLength(seq));
    if (seq->ItemDestroy)
    {
        seq->ItemDestroy(seq->data[index]);
    }
    seq->data[index] = item;
}

void SeqSoftSet(Seq *seq, size_t index, void *item)
{
    assert(seq != NULL);
    assert(index < SeqLength(seq));
    seq->data[index] = item;
}

void SeqAppend(Seq *seq, void *item)
{
    assert(seq != NULL);
    ExpandIfNeccessary(seq);

    seq->data[seq->length] = item;
    ++(seq->length);
}

void SeqAppendOnce(Seq *seq, void *item, SeqItemComparator Compare)
{
    assert(seq != NULL);
    if (SeqLookup(seq, item, Compare) == NULL)
    {
        SeqAppend(seq, item);
    }
    else
    {
        /* swallow the item anyway */
        if (seq->ItemDestroy != NULL)
        {
            seq->ItemDestroy(item);
        }
    }
}

void SeqAppendSeq(Seq *seq, const Seq *items)
{
    for (size_t i = 0; i < SeqLength(items); i++)
    {
        SeqAppend(seq, SeqAt(items, i));
    }
}

void SeqRemoveRange(Seq *seq, size_t start, size_t end)
{
    assert(seq != NULL);
    assert(end < seq->length);
    assert(start <= end);

    DestroyRange(seq, start, end);

    size_t rest_len = seq->length - end - 1;

    if (rest_len > 0)
    {
        memmove(seq->data + start, seq->data + end + 1, sizeof(void *) * rest_len);
    }

    seq->length -= end - start + 1;
}

void SeqRemove(Seq *seq, size_t index)
{
    SeqRemoveRange(seq, index, index);
}

void *SeqLookup(Seq *seq, const void *key, SeqItemComparator Compare)
{
    assert(seq != NULL);
    for (size_t i = 0; i < seq->length; i++)
    {
        if (Compare(key, seq->data[i], NULL) == 0)
        {
            return seq->data[i];
        }
    }

    return NULL;
}

void *SeqBinaryLookup(Seq *seq, const void *key, SeqItemComparator Compare)
{
    assert(seq != NULL);
    ssize_t index = SeqBinaryIndexOf(seq, key, Compare);
    if (index == -1)
    {
        return NULL;
    }
    else
    {
        return seq->data[index];
    }
}

ssize_t SeqIndexOf(Seq *seq, const void *key, SeqItemComparator Compare)
{
    assert(seq != NULL);
    for (size_t i = 0; i < seq->length; i++)
    {
        if (Compare(key, seq->data[i], NULL) == 0)
        {
            return i;
        }
    }

    return -1;
}

ssize_t SeqBinaryIndexOf(Seq *seq, const void *key, SeqItemComparator Compare)
{
    assert(seq != NULL);
    if (seq->length == 0)
    {
        return -1;
    }

    size_t low = 0;
    size_t high = seq->length;

    while (low < high)
    {
        // Invariant: low <= middle < high
        size_t middle = low + ((high - low) >> 1); // ">> 1" is division by 2.
        int result = Compare(key, seq->data[middle], NULL);
        if (result == 0)
        {
            return middle;
        }
        if (result > 0)
        {
            low = middle + 1;
        }
        else
        {
            high = middle;
        }
    }

    // Not found.
    return -1;
}

static void Swap(void **l, void **r)
{
    void *t = *l;

    *l = *r;
    *r = t;
}

// adopted from http://rosettacode.org/wiki/Sorting_algorithms/Quicksort#C
static void QuickSortRecursive(void **data, int n, SeqItemComparator Compare, void *user_data, size_t maxterm)
{
    if (n < 2)
    {
        return;
    }

    void *pivot = data[n / 2];
    void **l = data;
    void **r = data + n - 1;

    while (l <= r)
    {
        while (Compare(*l, pivot, user_data) < 0)
        {
            ++l;
        }
        while (Compare(*r, pivot, user_data) > 0)
        {
            --r;
        }
        if (l <= r)
        {
            Swap(l, r);
            ++l;
            --r;
        }
    }

    QuickSortRecursive(data, r - data + 1, Compare, user_data, maxterm + 1);
    QuickSortRecursive(l, data + n - l, Compare, user_data, maxterm + 1);
}

void SeqSort(Seq *seq, SeqItemComparator Compare, void *user_data)
{
    assert(seq != NULL);
    QuickSortRecursive(seq->data, seq->length, Compare, user_data, 0);
}

Seq *SeqSoftSort(const Seq *seq, SeqItemComparator compare, void *user_data)
{
    size_t length = SeqLength(seq);
    if (length == 0)
    {
        return SeqNew(0, NULL);
    }

    Seq *sorted_seq = SeqGetRange(seq, 0, length - 1);
    SeqSort(sorted_seq, compare, user_data);
    return sorted_seq;
}

void SeqSoftRemoveRange(Seq *seq, size_t start, size_t end)
{
    assert(seq != NULL);
    assert(end < seq->length);
    assert(start <= end);

    size_t rest_len = seq->length - end - 1;

    if (rest_len > 0)
    {
        memmove(seq->data + start, seq->data + end + 1, sizeof(void *) * rest_len);
    }

    seq->length -= end - start + 1;
}

void SeqClear(Seq *seq)
{
    if (SeqLength(seq) > 0)
    {
        SeqRemoveRange(seq, 0, SeqLength(seq) - 1);
    }
}

void SeqSoftRemove(Seq *seq, size_t index)
{
    SeqSoftRemoveRange(seq, index, index);
}

void SeqReverse(Seq *seq)
{
    assert(seq != NULL);
    for (size_t i = 0; i < (seq->length / 2); i++)
    {
        Swap(&seq->data[i], &seq->data[seq->length - 1 - i]);
    }
}

Seq *SeqSplit(Seq *seq, size_t index)
{
    assert(seq != NULL);
    size_t length = SeqLength(seq);
    assert(index <= length); // index > length is invalid
    if (index >= length)
    {
        // index == length is valid, return empty sequence
        // anything higher is error, but we will handle it anyway
        return SeqNew(1, seq->ItemDestroy);
    }

    Seq *ret = SeqGetRange(seq, index, length - 1);
    assert(ret != NULL); // Our indices should be valid
    SeqSoftRemoveRange(seq, index, length - 1);
    return ret;
}

size_t SeqLength(const Seq *seq)
{
    assert(seq != NULL);
    return seq->length;
}

void SeqShuffle(Seq *seq, unsigned int seed)
{
    assert(seq != NULL);
    if (SeqLength(seq) == 0)
    {
        return;
    }

    /* Store current random number state for being reset at the end of function */
    int rand_state = rand();

    srand(seed);
    for (size_t i = SeqLength(seq) - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);

        Swap(seq->data + i, seq->data + j);
    }

    /* Restore previous random number state */
    srand(rand_state);
}

Seq *SeqGetRange(const Seq *seq, size_t start, size_t end)
{
    assert(seq != NULL);

    if ((start > end) || (start >= seq->length) || (end >= seq->length))
    {
        return NULL;
    }

    Seq *sub = SeqNew(end - start + 1, seq->ItemDestroy);

    for (size_t i = start; i <= end; i++)
    {
        assert(i < SeqLength(seq));
        SeqAppend(sub, SeqAt(seq, i));
    }

    return sub;
}

void *const *SeqGetData(const Seq *seq)
{
    assert(seq != NULL);
    return seq->data;
}

void SeqRemoveNulls(Seq *seq)
{
    assert(seq != NULL);
    int length = SeqLength(seq);
    int from = 0;
    int to = 0;
    while (from < length)
    {
        if (seq->data[from] == NULL)
        {
            ++from; // Skip NULL elements
        }
        else
        {
            // Copy elements in place, DON'T use SeqSet, which will free()
            seq->data[to] = seq->data[from];
            ++from;
            ++to;
        }
    }
    seq->length = to;
}

Seq *SeqFromArgv(int argc, const char *const *const argv)
{
    assert(argc > 0);
    assert(argv != NULL);
    assert(argv[0] != NULL);

    Seq *args = SeqNew(argc, NULL);
    for (int i = 0; i < argc; ++i)
    {
        SeqAppend(args, (void *)argv[i]); // Discards const
    }
    return args;
}
