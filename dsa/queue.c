#include "queue.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>

/*
 * FIFO Queue for Pending Portfolio Operations & Scheduled Orders
 * Uses a linked list with front and rear pointers for O(1) enqueue and O(1) dequeue.
 */

void queue_init(TaskQueue *q) {
    if (!q) return;
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    q->next_task_id = 1;
}

int queue_enqueue(TaskQueue *q, TaskType type, const char *ticker, int quantity, double price, const char *timestamp) {
    if (!q || !ticker || !*ticker) return 0;
    QueueNode *node = (QueueNode *)calloc(1, sizeof(QueueNode));
    if (!node) return 0;

    node->task.task_id = q->next_task_id++;
    node->task.type = type;
    stock_normalize_ticker(ticker, node->task.ticker, sizeof(node->task.ticker));
    node->task.quantity = quantity;
    node->task.price = price;
    stock_copy_str(node->task.created_at, sizeof(node->task.created_at),
                   (timestamp && *timestamp) ? timestamp : "PENDING");
    node->next = NULL;

    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    q->size++;
    return 1;
}

int queue_dequeue(TaskQueue *q, PortfolioTask *task_out) {
    if (!q || !q->front) return 0;
    QueueNode *front_node = q->front;
    if (task_out) {
        *task_out = front_node->task;
    }
    q->front = front_node->next;
    if (!q->front) {
        q->rear = NULL;
    }
    free(front_node);
    if (q->size > 0) q->size--;
    return 1;
}

int queue_peek(const TaskQueue *q, PortfolioTask *task_out) {
    if (!q || !q->front) return 0;
    if (task_out) {
        *task_out = q->front->task;
    }
    return 1;
}

int queue_is_empty(const TaskQueue *q) {
    return (!q || q->size == 0);
}

size_t queue_size(const TaskQueue *q) {
    return q ? q->size : 0;
}

void queue_clear(TaskQueue *q) {
    if (!q) return;
    QueueNode *curr = q->front;
    while (curr) {
        QueueNode *next = curr->next;
        free(curr);
        curr = next;
    }
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}
