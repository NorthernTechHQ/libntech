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
#ifndef CFENGINE_RB_TREE_H
#define CFENGINE_RB_TREE_H

#include <stdbool.h>
#include <stddef.h>						/* size_t */

typedef struct RBTree_ RBTree;
typedef struct RBTreeIterator_ RBTreeIterator;

typedef void *RBTreeKeyCopyFn(const void *key);
typedef int RBTreeKeyCompareFn(const void *a, const void *b);
typedef void RBTreeKeyDestroyFn(void *key);
typedef void *RBTreeValueCopyFn(const void *key);
typedef int RBTreeValueCompareFn(const void *a, const void *b);
typedef void RBTreeValueDestroyFn(void *key);

typedef bool RBTreePredicate(const void *key, const void *value, void *user_data);

RBTree *RBTreeNew(RBTreeKeyCopyFn *key_copy,
                  RBTreeKeyCompareFn *key_compare,
                  RBTreeKeyDestroyFn *key_destroy,
                  RBTreeValueCopyFn *value_copy,
                  RBTreeValueCompareFn *value_compare,
                  RBTreeValueDestroyFn *value_destroy);

RBTree *RBTreeCopy(const RBTree *tree, RBTreePredicate *filter, void *user_data);

bool RBTreeEqual(const void *a, const void *b);
void RBTreeDestroy(void *rb_tree);

bool RBTreePut(RBTree *tree, const void *key, const void *value);
void *RBTreeGet(const RBTree *tree, const void *key);
bool RBTreeRemove(RBTree *tree, const void *key);
void RBTreeClear(RBTree *tree);
size_t RBTreeSize(const RBTree *tree);

RBTreeIterator *RBTreeIteratorNew(const RBTree *tree);
bool RBTreeIteratorNext(RBTreeIterator *iter, void **key, void **value);
void RBTreeIteratorDestroy(void *_rb_iter);

#endif
