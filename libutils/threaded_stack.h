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

#ifndef CFENGINE_THREADED_STACK_H
#define CFENGINE_THREADED_STACK_H

#include <platform.h>

typedef struct ThreadedStack_ ThreadedStack;

/**
  @brief Creates a new stack with specified capacity.
  @param [in] initial_capacity Initial capacity, defaults to 1.
  @param [in] ItemDestroy Function used to destroy data elements.
  */
ThreadedStack *ThreadedStackNew(size_t initial_capacity, void (*ItemDestroy) ());

/**
  @brief Destroys the stack and frees the memory it occupies.
  @param [in] stack The stack to destroy.
  @warning ThreadedStack should only be destroyed if all threads are joined
  */
void ThreadedStackDestroy(ThreadedStack *stack);

/**
  @brief Frees the memory allocated for the data pointer and the struct itself.
  @param [in] stack The stack to free.
  */
void ThreadedStackSoftDestroy(ThreadedStack *stack);

/**
  @brief Returns and removes the last element added to the stack.
  @note Will return NULL if stack is empty.
  @param [in] stack The stack to pop from.
  @return A pointer to the last data added.
  */
void *ThreadedStackPop(ThreadedStack *stack);

/**
  @brief Adds a new item on top of the stack.
  @param [in] stack The stack to push to.
  @param [in] item The item to push.
  */
void ThreadedStackPush(ThreadedStack *stack, void *item);

/**
  @brief Adds a new item on top of the stack and returns the current size.
  @param [in] stack The stack to push to.
  @param [in] item The item to push.
  @return The amount of elements in the stack.
  */
size_t ThreadedStackPushReportCount(ThreadedStack *stack, void *item);

/**
  @brief Get current number of items in stack.
  @note On NULL stack, returns 0.
  @param [in] stack The stack.
  @return The amount of elements in the stack.
  */
size_t ThreadedStackCount(ThreadedStack const *stack);

/**
  @brief Get current capacity of stack.
  @note On NULL stack, returns 0.
  @param [in] stack The stack.
  @return The current capacity of the stack.
  */
size_t ThreadedStackCapacity(ThreadedStack const *stack);

/**
  @brief Create a shallow copy of a given stack.
  @note This makes a new stack pointing to the same memory as the old stack.
  @param [in] stack The stack.
  @return A new stack pointing to the same data.
  */
ThreadedStack *ThreadedStackCopy(ThreadedStack const *stack);

/**
  @brief Checks if a stack is empty.
  @param [in] stack The stack.
  @return Returns true if stack is empty, false otherwise.
  */
bool ThreadedStackIsEmpty(ThreadedStack const *stack);

#endif
