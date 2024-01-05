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

#ifndef CFENGINE_ARRAY_MAP_PRIV_H
#define CFENGINE_ARRAY_MAP_PRIV_H

#include <map_common.h>

typedef struct
{
    MapKeyEqualFn equal_fn;
    MapDestroyDataFn destroy_key_fn;
    MapDestroyDataFn destroy_value_fn;
    MapKeyValue *values;
    short size;
} ArrayMap;

typedef struct
{
    ArrayMap *map;
    int pos;
} ArrayMapIterator;

ArrayMap *ArrayMapNew(MapKeyEqualFn equal_fn,
                      MapDestroyDataFn destroy_key_fn,
                      MapDestroyDataFn destroy_value_fn);

/**
 * @retval 0 if the limit of the array size has been reached,
 *           and no insertion took place.
 * @retval 1 if the key was found and the value was replaced.
 * @retval 2 if the key-value pair was not found and inserted as new.
 */
int ArrayMapInsert(ArrayMap *map, void *key, void *value);

bool ArrayMapRemove(ArrayMap *map, const void *key);
bool ArrayMapRemoveSoft(ArrayMap *map, const void *key);
MapKeyValue *ArrayMapGet(const ArrayMap *map, const void *key);
void ArrayMapClear(ArrayMap *map);
void ArrayMapSoftDestroy(ArrayMap *map);
void ArrayMapDestroy(ArrayMap *map);

/******************************************************************************/

ArrayMapIterator ArrayMapIteratorInit(ArrayMap *map);
MapKeyValue *ArrayMapIteratorNext(ArrayMapIterator *i);

#endif
