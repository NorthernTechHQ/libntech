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
#ifndef CFENGINE_RING_BUFFER_H
#define CFENGINE_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>						/* size_t */

typedef struct RingBuffer_ RingBuffer;
typedef struct RingBufferIterator_ RingBufferIterator;

RingBuffer *RingBufferNew(size_t capacity, void *(*copy)(const void *), void (*destroy)(void *));
void RingBufferDestroy(RingBuffer *buf);

void RingBufferAppend(RingBuffer *buf, void *item);
void RingBufferClear(RingBuffer *buf);

size_t RingBufferLength(const RingBuffer *buf);
bool RingBufferIsFull(const RingBuffer *buf);
const void *RingBufferHead(const RingBuffer *buf);
const void *RingBufferTail(const RingBuffer *buf);

RingBufferIterator *RingBufferIteratorNew(const RingBuffer *buf);
void RingBufferIteratorDestroy(RingBufferIterator *iter);
const void *RingBufferIteratorNext(RingBufferIterator *iter);


#endif
