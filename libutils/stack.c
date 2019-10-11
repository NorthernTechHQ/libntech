/*
  Copyright 2019 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

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
