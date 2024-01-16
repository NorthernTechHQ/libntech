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

#include <platform.h>
#include <threaded_stack.h>
#include <alloc.h>
#include <logging.h>
#include <mutex.h>
#include <pthread.h>
#include <stack_base.c>

/** @struct ThreadedStack_
  @brief A simple thread-safe stack data structure.

  Can push, pop, and copy. Also has functions for showing current stack size
  and capacity, and if a stack is empty. If the amount of pushed elements
  exceed the capacity, it will be multiplied by EXPAND_FACTOR and reallocated
  with the new capacity. When destroying the stack, destroys each element with
  the ItemDestroy function specified -- unless it is NULL -- and then proceeds
  to destroy the lock, before freeing the data array and the stack itself.
  */
struct ThreadedStack_ {
    Stack base;
    pthread_mutex_t *lock;            /**< Thread lock for accessing data. */
};

ThreadedStack *ThreadedStackNew(size_t initial_capacity, void (ItemDestroy) (void *item))
{
    ThreadedStack *stack = xmalloc(sizeof(ThreadedStack));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    int ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Failed to use error-checking mutexes for stack, "
            "falling back to normal ones (pthread_mutexattr_settype: %s)",
            GetErrorStrFromCode(ret));
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    }

    stack->lock = xmalloc(sizeof(pthread_mutex_t));
    ret = pthread_mutex_init(stack->lock, &attr);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Failed to initialize mutex (pthread_mutex_init: %s)",
            GetErrorStrFromCode(ret));
        pthread_mutexattr_destroy(&attr);
        free(stack);
        return NULL;
    }

    pthread_mutexattr_destroy(&attr);

    StackInit(&(stack->base), initial_capacity, ItemDestroy);

    return stack;
}

void ThreadedStackDestroy(ThreadedStack *stack)
{
    if (stack != NULL)
    {
        ThreadLock(stack->lock);
        DestroyRange(&(stack->base), 0, stack->base.size);
        ThreadUnlock(stack->lock);

        ThreadedStackSoftDestroy(stack);
    }
}

void ThreadedStackSoftDestroy(ThreadedStack *stack)
{
    if (stack != NULL)
    {
        if (stack->lock != NULL)
        {
            pthread_mutex_destroy(stack->lock);
            free(stack->lock);
        }
        free(stack->base.data);
        free(stack);
    }
}

void *ThreadedStackPop(ThreadedStack *stack)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    void *item = StackPop(&(stack->base));
    ThreadUnlock(stack->lock);

    return item;
}

void ThreadedStackPush(ThreadedStack *stack, void *item)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    StackPush(&(stack->base), item);
    ThreadUnlock(stack->lock);
}

size_t ThreadedStackPushReportCount(ThreadedStack *stack, void *item)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    const size_t size = StackPushReportCount(&(stack->base), item);
    ThreadUnlock(stack->lock);

    return size;
}

size_t ThreadedStackCount(ThreadedStack const *stack)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    size_t count = StackCount(&(stack->base));
    ThreadUnlock(stack->lock);

    return count;
}

size_t ThreadedStackCapacity(ThreadedStack const *stack)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    size_t capacity = StackCapacity(&(stack->base));
    ThreadUnlock(stack->lock);

    return capacity;
}

bool ThreadedStackIsEmpty(ThreadedStack const *stack)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);
    bool const empty = StackIsEmpty(&(stack->base));
    ThreadUnlock(stack->lock);

    return empty;
}

ThreadedStack *ThreadedStackCopy(ThreadedStack const *stack)
{
    assert(stack != NULL);

    ThreadLock(stack->lock);

    ThreadedStack *new_stack = xmemdup(stack, sizeof(ThreadedStack));
    new_stack->base.data = xmalloc(sizeof(void *) * stack->base.capacity);
    memcpy(new_stack->base.data, stack->base.data, sizeof(void *) * stack->base.size);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    int ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Failed to use error-checking mutexes for stack, "
            "falling back to normal ones (pthread_mutexattr_settype: %s)",
            GetErrorStrFromCode(ret));
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    }

    new_stack->lock = xmalloc(sizeof(pthread_mutex_t));
    ret = pthread_mutex_init(new_stack->lock, &attr);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Failed to initialize mutex (pthread_mutex_init: %s)",
            GetErrorStrFromCode(ret));
        free(new_stack->lock);
        free(new_stack);
        new_stack = NULL;
    }

    pthread_mutexattr_destroy(&attr);
    ThreadUnlock(stack->lock);

    return new_stack;
}
