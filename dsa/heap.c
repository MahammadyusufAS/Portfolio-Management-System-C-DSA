#include "heap.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Binary Min-Heap Priority Queue for Price Alerts
 * Orders alerts by urgency (priority_key):
 *   - Triggered alerts get priority_key <= 0.0 (highest priority, immediately at root)
 *   - Untriggered alerts get priority_key = |target_price - current_price| / target_price
 *     (closest to triggering is at the top of the heap).
 * Complexity: O(log n) push, O(log n) pop, O(1) peek.
 */

double heap_compute_priority(double current_price, double target_price, AlertCondition condition, int *triggered_out) {
    if (target_price <= 0.0) {
        if (triggered_out) *triggered_out = 0;
        return 1e9;
    }
    int trig = 0;
    if (condition == ALERT_ABOVE && current_price >= target_price) {
        trig = 1;
    } else if (condition == ALERT_BELOW && current_price > 0.0 && current_price <= target_price) {
        trig = 1;
    }
    if (triggered_out) *triggered_out = trig;
    if (trig) {
        /* Negative/zero key ensures triggered alerts bubble directly to root */
        return -fabs(current_price - target_price) / target_price;
    }
    if (current_price <= 0.0) {
        return target_price;
    }
    return fabs(target_price - current_price) / target_price;
}

static int alert_less(const PriceAlert *a, const PriceAlert *b) {
    if (fabs(a->priority_key - b->priority_key) > 1e-12) {
        return a->priority_key < b->priority_key;
    }
    if (fabs(a->target_price - b->target_price) > 1e-9) {
        return a->target_price < b->target_price;
    }
    return a->id < b->id;
}

static void swap_alerts(PriceAlert *a, PriceAlert *b) {
    PriceAlert tmp = *a;
    *a = *b;
    *b = tmp;
}

static void sift_up(AlertMinHeap *heap, size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (alert_less(&heap->items[idx], &heap->items[parent])) {
            swap_alerts(&heap->items[idx], &heap->items[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

static void sift_down(AlertMinHeap *heap, size_t idx) {
    while (1) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;

        if (left < heap->size && alert_less(&heap->items[left], &heap->items[smallest])) {
            smallest = left;
        }
        if (right < heap->size && alert_less(&heap->items[right], &heap->items[smallest])) {
            smallest = right;
        }
        if (smallest != idx) {
            swap_alerts(&heap->items[idx], &heap->items[smallest]);
            idx = smallest;
        } else {
            break;
        }
    }
}

void heap_init(AlertMinHeap *heap) {
    if (!heap) return;
    heap->items = NULL;
    heap->size = 0;
    heap->capacity = 0;
    heap->next_alert_id = 1;
}

int heap_push(AlertMinHeap *heap, const PriceAlert *alert) {
    if (!heap || !alert || !alert->ticker[0] || alert->target_price <= 0.0) return 0;
    if (heap->size >= heap->capacity) {
        size_t new_cap = heap->capacity ? heap->capacity * 2 : 16;
        PriceAlert *new_items = (PriceAlert *)realloc(heap->items, new_cap * sizeof(PriceAlert));
        if (!new_items) return 0;
        heap->items = new_items;
        heap->capacity = new_cap;
    }
    PriceAlert copy = *alert;
    if (copy.id <= 0) {
        copy.id = heap->next_alert_id++;
    } else if (copy.id >= heap->next_alert_id) {
        heap->next_alert_id = copy.id + 1;
    }
    stock_normalize_ticker(copy.ticker, copy.ticker, sizeof(copy.ticker));
    copy.priority_key = heap_compute_priority(copy.current_price, copy.target_price, copy.condition, &copy.triggered);

    size_t idx = heap->size++;
    heap->items[idx] = copy;
    sift_up(heap, idx);
    return 1;
}

int heap_pop(AlertMinHeap *heap, PriceAlert *alert_out) {
    if (!heap || heap->size == 0) return 0;
    if (alert_out) {
        *alert_out = heap->items[0];
    }
    heap->size--;
    if (heap->size > 0) {
        heap->items[0] = heap->items[heap->size];
        sift_down(heap, 0);
    }
    return 1;
}

int heap_peek(const AlertMinHeap *heap, PriceAlert *alert_out) {
    if (!heap || heap->size == 0) return 0;
    if (alert_out) {
        *alert_out = heap->items[0];
    }
    return 1;
}

int heap_remove_by_id(AlertMinHeap *heap, int alert_id) {
    if (!heap || heap->size == 0) return 0;
    for (size_t i = 0; i < heap->size; ++i) {
        if (heap->items[i].id == alert_id) {
            heap->size--;
            if (i < heap->size) {
                heap->items[i] = heap->items[heap->size];
                sift_up(heap, i);
                sift_down(heap, i);
            }
            return 1;
        }
    }
    return 0;
}

size_t heap_update_stock_price(AlertMinHeap *heap, const char *ticker, double new_price) {
    if (!heap || !ticker || heap->size == 0) return 0;
    size_t newly_triggered = 0;
    for (size_t i = 0; i < heap->size; ++i) {
        if (stock_stricmp(heap->items[i].ticker, ticker) == 0) {
            heap->items[i].current_price = new_price;
            int trig = 0;
            heap->items[i].priority_key = heap_compute_priority(
                new_price, heap->items[i].target_price, heap->items[i].condition, &trig);
            heap->items[i].triggered = trig;
            if (trig) newly_triggered++;
        }
    }
    /* Floyd's bottom-up O(n) heapify */
    for (size_t i = heap->size / 2; i > 0; --i) {
        sift_down(heap, i - 1);
    }
    return newly_triggered;
}

size_t heap_collect_triggered(const AlertMinHeap *heap, PriceAlert *out_alerts, size_t max_out) {
    if (!heap || !out_alerts || max_out == 0) return 0;
    size_t count = 0;
    for (size_t i = 0; i < heap->size && count < max_out; ++i) {
        if (heap->items[i].triggered) {
            out_alerts[count++] = heap->items[i];
        }
    }
    return count;
}

int heap_is_valid(const AlertMinHeap *heap) {
    if (!heap) return 1;
    for (size_t i = 0; i < heap->size; ++i) {
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;
        if (left < heap->size && alert_less(&heap->items[left], &heap->items[i])) return 0;
        if (right < heap->size && alert_less(&heap->items[right], &heap->items[i])) return 0;
    }
    return 1;
}

void heap_clear(AlertMinHeap *heap) {
    if (!heap) return;
    free(heap->items);
    heap->items = NULL;
    heap->size = 0;
    heap->capacity = 0;
}
