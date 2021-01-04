/*
  Copyright 2021 Northern.tech AS

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

/** @file stack_base.c
 * @brief Shared code between threaded_stack.c and stack.c
 *
 * This file should be included, not compiled separately.
 */


#include <stack.h>

#define EXPAND_FACTOR     2
#define DEFAULT_CAPACITY 16

/** @struct Stack_
  @brief A simple stack data structure.

  Can push, pop, and copy. Also has functions for showing current stack size
  and capacity, and if a stack is empty. If the amount of pushed elements
  exceed the capacity, it will be multiplied by EXPAND_FACTOR and reallocated
  with the new capacity. When destroying the stack, destroys each element with
  the ItemDestroy function specified before freeing the data array and the
  stack itself.
  */
struct Stack_ {
    void (*ItemDestroy) (void *item); /**< Data-specific destroy function. */
    void **data;                      /**< Internal array of elements.     */
    size_t size;                      /**< Amount of elements in stack.    */
    size_t capacity;                  /**< Current memory allocated.       */
};

static void StackInit(Stack *stack, size_t initial_capacity, void (ItemDestroy) (void *item))
{
    assert(stack != NULL);

    if (initial_capacity == 0)
    {
        initial_capacity = DEFAULT_CAPACITY;
    }

    stack->capacity = initial_capacity;
    stack->size = 0;
    stack->data = xcalloc(initial_capacity, sizeof(void *));
    stack->ItemDestroy = ItemDestroy;
}

/**
  @brief Destroys data in range.
  @note Assumes that locks are acquired.
  @param [in] stack Pointer to struct.
  @param [in] start Start position to destroy from.
  @param [in] end Where to stop.
  */
static void DestroyRange(Stack *stack, size_t start, size_t end)
{
    assert(stack != NULL);

    if (start > stack->capacity || end > stack->capacity)
    {
        return;
    }

    if (stack->ItemDestroy)
    {
        for (size_t i = start; i < end; i++)
        {
            stack->ItemDestroy(stack->data[i]);
        }
    }
}
