#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

typedef enum {
    TASK_BUY_ORDER = 1,
    TASK_SELL_ORDER = 2,
    TASK_PRICE_UPDATE = 3,
    TASK_PORTFOLIO_SNAPSHOT = 4
} TaskType;

typedef struct {
    unsigned long task_id;
    TaskType type;
    char ticker[24];
    int quantity;
    double price;
    char created_at[32];
} PortfolioTask;

typedef struct QueueNode {
    PortfolioTask task;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    size_t size;
    unsigned long next_task_id;
} TaskQueue;

void queue_init(TaskQueue *q);
int queue_enqueue(TaskQueue *q, TaskType type, const char *ticker, int quantity, double price, const char *timestamp);
int queue_dequeue(TaskQueue *q, PortfolioTask *task_out);
int queue_peek(const TaskQueue *q, PortfolioTask *task_out);
int queue_is_empty(const TaskQueue *q);
size_t queue_size(const TaskQueue *q);
void queue_clear(TaskQueue *q);

#endif /* QUEUE_H */
