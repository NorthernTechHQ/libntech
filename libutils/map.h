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

#ifndef CFENGINE_MAP_H
#define CFENGINE_MAP_H

#include <hash_map_priv.h>
#include <array_map_priv.h>

/*
 * Map structure. Details are encapsulated.
 */
typedef struct Map_ Map;

Map *MapNew(MapHashFn hash_fn,
            MapKeyEqualFn equal_fn,
            MapDestroyDataFn destroy_key_fn,
            MapDestroyDataFn destroy_value_fn);

/**
 * Insert a key-value pair in the map.
 * If the key is in the map, value get replaced. Old value is destroyed.
 *
 * @retval true if key exists already.
 */
bool MapInsert(Map *map, void *key, void *value);

/*
 * Returns whether the key is in the map.
 */
bool MapHasKey(const Map *map, const void *key);

/*
 * Returns the value if the key is in map, NULL otherwise. To distinguish
 * between NULL as a value and NULL as a lack of entry, use MapHasKey.
 */
void *MapGet(Map *map, const void *key);

/*
 * Remove key/value pair from the map. Returns 'true' if key was present in the
 * map.
 */
bool MapRemove(Map *map, const void *key);

size_t MapSize(const Map *map);

/*
 * MapIterator i = MapIteratorInit(map);
 * MapKeyValue *item;
 * while ((item = MapIteratorNext(&i)))
 * {
 *     // do something with item->key, item->value
 * }
 */


typedef struct
{
    bool is_array;
    union
    {
        ArrayMapIterator arraymap_iter;
        HashMapIterator hashmap_iter;
    };
} MapIterator;

MapIterator MapIteratorInit(Map *map);

MapKeyValue *MapIteratorNext(MapIterator *i);

/*
 * Clear the whole map
 */
void MapClear(Map *map);

/*
 * Destroy the map object.
 */
void MapDestroy(Map *map);

/*
 * Destroy the map object without removing values.
 */
void MapSoftDestroy(Map *map);

/**
 * Returns whether the two maps contain the same keys.
 * The values DO NOT have to be equal, just the keys.
 */
bool MapContainsSameKeys(const Map *map1, const Map *map2);

void MapPrintStats(const Map *map, FILE *f);


#define TYPED_MAP_DECLARE(Prefix, KeyType, ValueType)                   \
    typedef struct                                                      \
    {                                                                   \
        Map *impl;                                                      \
    } Prefix##Map;                                                      \
                                                                        \
    Prefix##Map *Prefix##MapNew(void);                                  \
    bool Prefix##MapInsert(const Prefix##Map *map, KeyType key, ValueType value); \
    bool Prefix##MapHasKey(const Prefix##Map *map, const KeyType key);  \
    ValueType Prefix##MapGet(const Prefix##Map *map, const KeyType key); \
    bool Prefix##MapRemove(const Prefix##Map *map, const KeyType key);  \
    void Prefix##MapClear(Prefix##Map *map);                            \
    size_t Prefix##MapSize(const Prefix##Map *map);                            \
    void Prefix##MapDestroy(Prefix##Map *map);                          \
    void Prefix##MapSoftDestroy(Prefix##Map *map);                          \
    bool Prefix##MapContainsSameKeys(const Prefix##Map *map1, const Prefix##Map *map2); \
    void Prefix##MapPrintStats(const Prefix##Map *map, FILE *f); \

#define TYPED_MAP_DEFINE(Prefix, KeyType, ValueType, hash_fn, equal_fn, \
                         destroy_key_fn, destroy_value_fn)              \
                                                                        \
    Prefix##Map *Prefix##MapNew(void)                                   \
    {                                                                   \
        Prefix##Map *map = xcalloc(1, sizeof(Prefix##Map));             \
        map->impl = MapNew(hash_fn, equal_fn,                           \
                           destroy_key_fn, destroy_value_fn);           \
        return map;                                                     \
    }                                                                   \
                                                                        \
    bool Prefix##MapInsert(const Prefix##Map *map, KeyType key, ValueType value) \
    {                                                                   \
        assert(map);                                                    \
        return MapInsert(map->impl, key, value);                        \
    }                                                                   \
                                                                        \
    bool Prefix##MapHasKey(const Prefix##Map *map, const KeyType key)   \
    {                                                                   \
        assert(map);                                                    \
        return MapHasKey(map->impl, key);                               \
    }                                                                   \
                                                                        \
    ValueType Prefix##MapGet(const Prefix##Map *map, const KeyType key) \
    {                                                                   \
        assert(map);                                                    \
        return MapGet(map->impl, key);                                  \
    }                                                                   \
                                                                        \
    bool Prefix##MapRemove(const Prefix##Map *map, const KeyType key)   \
    {                                                                   \
        assert(map);                                                    \
        return MapRemove(map->impl, key);                               \
    }                                                                   \
                                                                        \
    void Prefix##MapClear(Prefix##Map *map)                             \
    {                                                                   \
        assert(map);                                                    \
        MapClear(map->impl);                                            \
    }                                                                   \
                                                                        \
    size_t Prefix##MapSize(const Prefix##Map *map)                      \
    {                                                                   \
        assert(map);                                                    \
        return MapSize(map->impl);                                      \
    }                                                                   \
                                                                        \
    void Prefix##MapDestroy(Prefix##Map *map)                           \
    {                                                                   \
        if (map != NULL)                                                \
        {                                                               \
          MapDestroy(map->impl);                                        \
          free(map);                                                    \
        }                                                               \
    }                                                                   \
                                                                        \
    void Prefix##MapSoftDestroy(Prefix##Map *map)                       \
    {                                                                   \
        if (map != NULL)                                                \
        {                                                               \
          MapSoftDestroy(map->impl);                                    \
          free(map);                                                    \
        }                                                               \
    }                                                                   \
                                                                        \
    bool Prefix##MapContainsSameKeys(const Prefix##Map *map1, const Prefix##Map *map2) \
    {                                                                   \
        assert(map1);                                                   \
        assert(map2);                                                   \
        return MapContainsSameKeys(map1->impl, map2->impl);             \
    }                                                                   \
                                                                        \
    void Prefix##MapPrintStats(const Prefix##Map *map, FILE *f)         \
    {                                                                   \
        assert(map);                                                    \
        return MapPrintStats(map->impl, f);                             \
    }                                                                   \

TYPED_MAP_DECLARE(String, char *, char *)

#endif
