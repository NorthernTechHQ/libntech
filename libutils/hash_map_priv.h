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

#ifndef CFENGINE_HASH_MAP_PRIV_H
#define CFENGINE_HASH_MAP_PRIV_H

#include <stddef.h>    // size_t
#include <stdio.h>     // FILE
#include <map_common.h>

typedef struct BucketListItem_
{
    MapKeyValue value;
    struct BucketListItem_ *next;
} BucketListItem;

typedef struct
{
    MapHashFn hash_fn;
    MapKeyEqualFn equal_fn;
    MapDestroyDataFn destroy_key_fn;
    MapDestroyDataFn destroy_value_fn;
    BucketListItem **buckets;
    size_t size;
    size_t init_size;
    size_t load;
    size_t max_threshold;
    size_t min_threshold;
} HashMap;

typedef struct
{
    HashMap *map;
    BucketListItem *cur;
    size_t bucket;
} HashMapIterator;

HashMap *HashMapNew(MapHashFn hash_fn, MapKeyEqualFn equal_fn,
                    MapDestroyDataFn destroy_key_fn,
                    MapDestroyDataFn destroy_value_fn,
                    size_t init_size);

bool HashMapInsert(HashMap *map, void *key, void *value);
bool HashMapRemove(HashMap *map, const void *key);
bool HashMapRemoveSoft(HashMap *map, const void *key);
MapKeyValue *HashMapGet(const HashMap *map, const void *key);
void HashMapClear(HashMap *map);
void HashMapSoftDestroy(HashMap *map);
void HashMapDestroy(HashMap *map);
void HashMapPrintStats(const HashMap *hmap, FILE *f);

/******************************************************************************/

HashMapIterator HashMapIteratorInit(HashMap *m);
MapKeyValue *HashMapIteratorNext(HashMapIterator *i);

#endif
