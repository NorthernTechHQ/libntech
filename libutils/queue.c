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

#include <alloc.h>
#include <queue.h>
#include <refcount.h>

typedef struct QueueNode_ {
    void *data;                 /*!< Pointer to the stored element */
    struct QueueNode_ *next;     /*!< Next element in the queue or NULL */
    struct QueueNode_ *previous; /*!< Pointer to the previous element or NULL */
} QueueNode;

struct Queue_ {
    size_t node_count;      /*!< Number of elements in the queue */
    QueueItemDestroy *destroy;
    QueueNode *head; /*!< Pointer to the start of the queue */
    QueueNode *tail; /*!< Pointer to the end of the queue */
};

Queue *QueueNew(QueueItemDestroy *item_destroy)
{
    Queue *queue = xcalloc(1, sizeof(Queue));

    queue->destroy = item_destroy;

    return queue;
}

void QueueDestroy(Queue *q)
{
    if (q)
    {
        QueueNode *current = q->head;
        while (current)
        {
            QueueNode *next = current->next;
            if (q->destroy)
            {
                q->destroy(current->data);
            }
            free(current);
            current = next;
        }

        free(q);
    }
}

static QueueNode *QueueNodeNew(void *element)
{
    QueueNode *node = xmalloc(sizeof(QueueNode));

    node->data = element;
    node->previous = NULL;
    node->next = NULL;

    return node;
}

void QueueEnqueue(Queue *q, void *element)
{
    assert(q);

    QueueNode *node = QueueNodeNew(element);

    if (q->tail)
    {
        q->tail->next = node;
        node->previous = q->tail;
        q->tail = node;
    }
    else
    {
        q->tail = node;
        q->head = node;
    }

    ++q->node_count;
}

void *QueueDequeue(Queue *q)
{
    assert(q);

    QueueNode *node = q->head;
    void *data = node->data;
    q->head = node->next;
    if (q->head)
    {
        q->head->previous = NULL;
    }
    else
    {
        /* Empty queue */
        q->head = NULL;
        q->tail = NULL;
    }
    --q->node_count;
    /* Free the node */
    free(node);
    /* Return the data */
    return data;
}

void *QueueHead(Queue *q)
{
    assert(q);
    return q->head->data;
}

int QueueCount(const Queue *q)
{
    assert(q);
    return q->node_count;
}

bool QueueIsEmpty(const Queue *q)
{
    assert(q);
    return q->node_count == 0;
}
