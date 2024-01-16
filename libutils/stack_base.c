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
