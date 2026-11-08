#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

typedef enum {
    ALERT_ABOVE = 1,
    ALERT_BELOW = 2
} AlertCondition;

typedef struct {
    int id;
    char ticker[24];
    double target_price;
    double current_price;
    AlertCondition condition;
    double priority_key; /* Lower key = higher urgency (0.0 when triggered, else relative distance / target_price) */
    int triggered;
    char notes[64];
} PriceAlert;

typedef struct {
    PriceAlert *items;
    size_t size;
    size_t capacity;
    int next_alert_id;
} AlertMinHeap;

void heap_init(AlertMinHeap *heap);
double heap_compute_priority(double current_price, double target_price, AlertCondition condition, int *triggered_out);
int heap_push(AlertMinHeap *heap, const PriceAlert *alert);
int heap_pop(AlertMinHeap *heap, PriceAlert *alert_out);
int heap_peek(const AlertMinHeap *heap, PriceAlert *alert_out);
int heap_remove_by_id(AlertMinHeap *heap, int alert_id);
size_t heap_update_stock_price(AlertMinHeap *heap, const char *ticker, double new_price);
size_t heap_collect_triggered(const AlertMinHeap *heap, PriceAlert *out_alerts, size_t max_out);
int heap_is_valid(const AlertMinHeap *heap);
void heap_clear(AlertMinHeap *heap);

#endif /* HEAP_H */
