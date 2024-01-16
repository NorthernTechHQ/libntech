/*
  Copyright 2024 Northern.tech AS

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
#include <set.h>
#include <string.h> // strlen()

#include <alloc.h>
#include <string_lib.h>
#include <buffer.h>

TYPED_SET_DEFINE(String, char *,
                 StringHash_untyped, StringEqual_untyped, free)

Set *SetNew(MapHashFn element_hash_fn,
            MapKeyEqualFn element_equal_fn,
            MapDestroyDataFn element_destroy_fn)
{
    return MapNew(element_hash_fn, element_equal_fn, element_destroy_fn, NULL);
}

void SetDestroy(Set *set)
{
    MapDestroy(set);
}

void SetAdd(Set *set, void *element)
{
    assert(set != NULL);
    MapInsert(set, element, element);
}

bool SetContains(const Set *set, const void *element)
{
    assert(set != NULL);
    return MapHasKey(set, element);
}

bool SetRemove(Set *set, const void *element)
{
    assert(set != NULL);
    return MapRemove(set, element);
}

void SetClear(Set *set)
{
    assert(set != NULL);
    MapClear(set);
}

size_t SetSize(const Set *set)
{
    assert(set != NULL);
    return MapSize(set);
}

bool SetIsEqual(const Set *set1, const Set *set2)
{
    assert(set1 != NULL);
    assert(set2 != NULL);
    return MapContainsSameKeys(set1, set2);
}

SetIterator SetIteratorInit(Set *set)
{
    assert(set != NULL);
    return MapIteratorInit(set);
}

void *SetIteratorNext(SetIterator *i)
{
    MapKeyValue *kv = MapIteratorNext(i);
    return kv ? kv->key : NULL;
}

void SetJoin(Set *set, Set *otherset, SetElementCopyFn copy_function)
{
    assert(set != NULL);
    assert(otherset != NULL);
    if (set == otherset)
        return;

    SetIterator si = SetIteratorInit(otherset);
    void *ptr = NULL;

    for (ptr = SetIteratorNext(&si); ptr != NULL; ptr = SetIteratorNext(&si))
    {
        if (copy_function != NULL)
        {
            ptr = copy_function(ptr);
        }
        SetAdd(set, ptr);
    }
}

Buffer *StringSetToBuffer(StringSet *set, const char delimiter)
{
    assert(set != NULL);

    Buffer *buf = BufferNew();
    StringSetIterator it = StringSetIteratorInit(set);
    const char *element = NULL;
    int pos = 0;
    int size = StringSetSize(set);
    char minibuf[2];

    minibuf[0] = delimiter;
    minibuf[1] = '\0';

    while ((element = StringSetIteratorNext(&it)))
    {
        BufferAppend(buf, element, strlen(element));
        if (pos < size-1)
        {
            BufferAppend(buf, minibuf, sizeof(char));
        }

        pos++;
    }

    return buf;
}

void StringSetAddSplit(StringSet *set, const char *str, char delimiter)
{
    assert(set != NULL);
    if (str) // TODO: remove this inconsistency, add assert(str)
    {
        const char *prev = str;
        const char *cur = str;

        while (*cur != '\0')
        {
            if (*cur == delimiter)
            {
                size_t len = cur - prev;
                if (len > 0)
                {
                    StringSetAdd(set, xstrndup(prev, len));
                }
                else
                {
                    StringSetAdd(set, xstrdup(""));
                }
                prev = cur + 1;
            }

            cur++;
        }

        if (cur > prev)
        {
            StringSetAdd(set, xstrndup(prev, cur - prev));
        }
    }
}

StringSet *StringSetFromString(const char *str, char delimiter)
{
    StringSet *set = StringSetNew();

    StringSetAddSplit(set, str, delimiter);

    return set;
}

JsonElement *StringSetToJson(const StringSet *set)
{
    assert(set != NULL);

    JsonElement *arr = JsonArrayCreate(StringSetSize(set));
    StringSetIterator it = StringSetIteratorInit((StringSet *)set);
    const char *el = NULL;

    while ((el = StringSetIteratorNext(&it)))
    {
        JsonArrayAppendString(arr, el);
    }

    return arr;
}

static bool VisitJsonArrayFirst(ARG_UNUSED JsonElement *element, void *data)
{
    /* We only want one array with items, so just make sure there are no items
     * in the string set yet. This doesn't fail if there is a nested array as
     * the first item in the top-level array, but let's just live with that for
     * the sake of simplicity. */
    StringSet *set = data;
    return (StringSetSize(set) == 0);
}

static bool AddArrayItemToStringSet(JsonElement *element, void *data)
{
    char *element_str = JsonPrimitiveToString(element);
    StringSet *set = data;
    if ((element_str != NULL) && (set != NULL))
    {
        StringSetAdd(set, element_str);
        return true;
    }
    return false;
}

StringSet *JsonArrayToStringSet(const JsonElement *array)
{
    assert(array != NULL);

    if (JsonGetType(array) != JSON_TYPE_ARRAY)
    {
        return NULL;
    }

    StringSet *ret = StringSetNew();

    /* We know our visitor functions don't modify the given array so we can
     * safely type-cast the array to JsonElement* without 'const'. */
    bool success = JsonWalk((JsonElement *) array, JsonErrorVisitor, VisitJsonArrayFirst,
                            AddArrayItemToStringSet, ret);
    if (!success)
    {
        StringSetDestroy(ret);
        return NULL;
    }
    return ret;
}
