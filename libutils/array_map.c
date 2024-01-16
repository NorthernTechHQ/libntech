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
#include <array_map_priv.h>
#include <alloc.h>

/* FIXME: make configurable and move to map.c */
#define TINY_LIMIT 14

ArrayMap *ArrayMapNew(MapKeyEqualFn equal_fn,
                      MapDestroyDataFn destroy_key_fn,
                      MapDestroyDataFn destroy_value_fn)
{
    ArrayMap *map = xcalloc(1, sizeof(ArrayMap));
    map->equal_fn = equal_fn;
    map->destroy_key_fn = destroy_key_fn;
    map->destroy_value_fn = destroy_value_fn;
    map->values = xcalloc(1, sizeof(MapKeyValue) * TINY_LIMIT);
    return map;
}

int ArrayMapInsert(ArrayMap *map, void *key, void *value)
{
    if (map->size == TINY_LIMIT)
    {
        return 0;
    }

    for (int i = 0; i < map->size; ++i)
    {
        if (map->equal_fn(map->values[i].key, key))
        {
            /* Replace the key with the new one despite those two being the
             * same, since the new key might be referenced somewhere inside
             * the new value. */
            map->destroy_key_fn(map->values[i].key);
            map->destroy_value_fn(map->values[i].value);
            map->values[i].key   = key;
            map->values[i].value = value;
            return 1;
        }
    }

    map->values[map->size++] = (MapKeyValue) { key, value };
    return 2;
}

bool ArrayMapRemove(ArrayMap *map, const void *key)
{
    for (int i = 0; i < map->size; ++i)
    {
        if (map->equal_fn(map->values[i].key, key))
        {
            map->destroy_key_fn(map->values[i].key);
            map->destroy_value_fn(map->values[i].value);

            memmove(map->values + i, map->values + i + 1,
                    sizeof(MapKeyValue) * (map->size - i - 1));

            map->size--;
            return true;
        }
    }
    return false;
}

MapKeyValue *ArrayMapGet(const ArrayMap *map, const void *key)
{
    for (int i = 0; i < map->size; ++i)
    {
        if (map->equal_fn(map->values[i].key, key))
        {
            return map->values + i;
        }
    }
    return NULL;
}

void ArrayMapClear(ArrayMap *map)
{
    for (int i = 0; i < map->size; ++i)
    {
        map->destroy_key_fn(map->values[i].key);
        map->destroy_value_fn(map->values[i].value);
    }
    map->size = 0;
}

void ArrayMapSoftDestroy(ArrayMap *map)
{
    if (map)
    {
        for (int i = 0; i < map->size; ++i)
        {
            map->destroy_key_fn(map->values[i].key);
        }
        map->size = 0;

        free(map->values);
        free(map);
    }
}

void ArrayMapDestroy(ArrayMap *map)
{
    if (map)
    {
        ArrayMapClear(map);
        free(map->values);
        free(map);
    }
}

/******************************************************************************/

ArrayMapIterator ArrayMapIteratorInit(ArrayMap *map)
{
    return (ArrayMapIterator) { map, 0 };
}

MapKeyValue *ArrayMapIteratorNext(ArrayMapIterator *i)
{
    if (i->pos >= i->map->size)
    {
        return NULL;
    }
    else
    {
        return &i->map->values[i->pos++];
    }
}
