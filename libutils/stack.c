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
#include <alloc.h>
#include <logging.h>
#include <stack_base.c>

Stack *StackNew(size_t initial_capacity, void (ItemDestroy) (void *item))
{
    Stack *stack = xmalloc(sizeof(Stack));

    StackInit(stack, initial_capacity, ItemDestroy);

    return stack;
}

void StackDestroy(Stack *stack)
{
    if (stack != NULL)
    {
        DestroyRange(stack, 0, stack->size);

        StackSoftDestroy(stack);
    }
}

void StackSoftDestroy(Stack *stack)
{
    if (stack != NULL)
    {
        free(stack->data);
        free(stack);
    }
}

void *StackPop(Stack *stack)
{
    assert(stack != NULL);

    size_t size = stack->size;
    void *item = NULL;

    if (size > 0)
    {
        size--;
        item = stack->data[size];

        stack->data[size] = NULL;
        stack->size = size;
    }

    return item;
}

void *StackTop(Stack *stack)
{
    assert(stack != NULL);

    size_t size = stack->size;

    if (size > 0)
    {
        return stack->data[size-1];
    }

    return NULL;
}

/**
  @brief Expands capacity of stack.
  @note Assumes that locks are acquired.
  @param [in] stack Pointer to struct.
  */
static void ExpandIfNecessary(Stack *stack)
{
    assert(stack != NULL);
    assert(stack->size <= stack->capacity);

    if (stack->size == stack->capacity)
    {
        stack->capacity *= EXPAND_FACTOR;
        stack->data = xrealloc(stack->data, sizeof(void *) * stack->capacity);
    }
}

void StackPush(Stack *stack, void *item)
{
    assert(stack != NULL);

    ExpandIfNecessary(stack);
    stack->data[stack->size++] = item;
}

size_t StackPushReportCount(Stack *stack, void *item)
{
    assert(stack != NULL);

    ExpandIfNecessary(stack);
    stack->data[stack->size++] = item;
    size_t size = stack->size;

    return size;
}

size_t StackCount(Stack const *stack)
{
    assert(stack != NULL);

    size_t count = stack->size;

    return count;
}

size_t StackCapacity(Stack const *stack)
{
    assert(stack != NULL);

    size_t capacity = stack->capacity;

    return capacity;
}

bool StackIsEmpty(Stack const *stack)
{
    assert(stack != NULL);

    bool const empty = (stack->size == 0);

    return empty;
}

Stack *StackCopy(Stack const *stack)
{
    assert(stack != NULL);

    Stack *new_stack = xmemdup(stack, sizeof(Stack));
    new_stack->data = xmalloc(sizeof(void *) * stack->capacity);
    memcpy(new_stack->data, stack->data, sizeof(void *) * stack->size);

    return new_stack;
}
