#include "linkedlist.h"
#include "stack.h"
#include "queue.h"
#include "heap.h"
#include "sorting.h"
#include "graph.h"
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int main(void) {
    printf("=== Running Unit Tests: Linked List, Stack, Queue, Min-Heap, Sorting & Graph ===\n");

    /* 1. Doubly Linked List Tests */
    TransactionList list;
    dll_init(&list);
    assert(list.size == 0);

    assert(transaction_record(&list, 1, 1, 1, "RELIANCE", 1, 10, 1400.0, 0.0, "2026-11-01 10:00:00") != NULL);
    assert(transaction_record(&list, 2, 1, 2, "TCS",      1,  5, 3100.0, 0.0, "2026-11-02 11:00:00") != NULL);
    assert(transaction_record(&list, 3, 1, 1, "RELIANCE", 0,  4, 1500.0, 400.0, "2026-11-03 14:00:00") != NULL);
    assert(list.size == 3);

    /* Forward & Backward Traversal Verification */
    assert(list.head->id == 1 && list.tail->id == 3);
    assert(list.tail->prev->id == 2 && list.head->next->id == 2);

    const TransactionNode *filtered[16];
    size_t n = dll_filter(&list, "RELIANCE", "ALL", "", "", filtered, 16, 0);
    assert(n == 2 && filtered[0]->id == 1 && filtered[1]->id == 3);

    n = dll_filter(&list, "", "SELL", "2026-11-02", "2026-11-04", filtered, 16, 1);
    assert(n == 1 && filtered[0]->id == 3);

    TransactionSummary sum;
    transaction_compute_summary(&list, &sum);
    assert(sum.total_transactions == 3 && sum.buy_count == 2 && sum.sell_count == 1);
    assert(fabs(sum.net_realized_pl - 400.0) < 1e-6);

    assert(dll_remove_by_id(&list, 2) == 1);
    assert(list.size == 2 && list.head->next == list.tail && list.tail->prev == list.head);
    dll_clear(&list);

    /* 2. LIFO UndoStack Tests */
    UndoStack st;
    stack_init(&st);
    assert(stack_is_empty(&st) == 1);
    for (int i = 1; i <= 20; ++i) {
        UndoRecord r = {0};
        r.transaction_id = (unsigned long)i;
        r.quantity_delta = i * 5;
        assert(stack_push(&st, &r) == 1);
    }
    assert(st.size == 20);
    UndoRecord top;
    assert(stack_peek(&st, &top) == 1 && top.transaction_id == 20);
    assert(stack_pop(&st, &top) == 1 && top.transaction_id == 20);
    assert(st.size == 19);
    stack_clear(&st);

    /* 3. FIFO TaskQueue Tests */
    TaskQueue q;
    queue_init(&q);
    assert(queue_is_empty(&q) == 1);
    assert(queue_enqueue(&q, TASK_BUY_ORDER, "INFY", 15, 1520.0, "2026-11-08 09:00:00") == 1);
    assert(queue_enqueue(&q, TASK_SELL_ORDER, "TCS", 5, 3250.0, "2026-11-08 09:05:00") == 1);
    assert(queue_size(&q) == 2);

    PortfolioTask task;
    assert(queue_dequeue(&q, &task) == 1 && strcmp(task.ticker, "INFY") == 0 && task.quantity == 15);
    assert(queue_dequeue(&q, &task) == 1 && strcmp(task.ticker, "TCS") == 0 && task.quantity == 5);
    assert(queue_is_empty(&q) == 1);
    queue_clear(&q);

    /* 4. Min-Heap Priority Queue Price Alerts Tests */
    AlertMinHeap heap;
    heap_init(&heap);
    PriceAlert a1 = { .id = 1, .ticker = "RELIANCE", .target_price = 1600.0, .current_price = 1450.0, .condition = ALERT_ABOVE };
    PriceAlert a2 = { .id = 2, .ticker = "TCS",      .target_price = 3210.0, .current_price = 3200.0, .condition = ALERT_ABOVE };
    PriceAlert a3 = { .id = 3, .ticker = "ITC",      .target_price = 435.0,  .current_price = 440.0,  .condition = ALERT_ABOVE }; /* Already triggered! */

    assert(heap_push(&heap, &a1) == 1);
    assert(heap_push(&heap, &a2) == 1);
    assert(heap_push(&heap, &a3) == 1);
    assert(heap_is_valid(&heap) == 1);

    /* Triggered alert a3 must be at the root of the Min-Heap */
    PriceAlert root;
    assert(heap_peek(&heap, &root) == 1 && root.id == 3 && root.triggered == 1);
    assert(heap_pop(&heap, &root) == 1 && root.id == 3);
    /* Next closest alert is TCS (3200 -> 3210, ~0.31% away) */
    assert(heap_peek(&heap, &root) == 1 && root.id == 2);
    heap_clear(&heap);

    /* 5. Sorting Algorithms (Merge Sort, Quick Sort, Insertion Sort) */
    StockArray arr;
    stock_array_init(&arr, 8);
    const char *tickers[5] = { "WIPRO", "AXISBANK", "RELIANCE", "INFY", "TCS" };
    const double prices[5] = { 550.0, 1180.0, 1450.0, 1550.0, 3200.0 };
    const int qtys[5]      = { 50, 20, 30, 10, 15 };
    for (int i = 0; i < 5; ++i) {
        Stock s = {0};
        s.id = i + 1;
        strcpy(s.ticker, tickers[i]);
        strcpy(s.name, tickers[i]);
        s.current_price = prices[i];
        s.quantity = qtys[i];
        s.average_cost = prices[i] * 0.9;
        stock_recalculate_metrics(&s);
        stock_array_push(&arr, &s, NULL);
    }

    SortItem m_items[5], q_items[5], i_items[5];
    size_t cnt = 0;
    sort_build_items(&arr, SORT_BY_MARKET_VALUE, m_items, 5, &cnt);
    sort_build_items(&arr, SORT_BY_MARKET_VALUE, q_items, 5, &cnt);
    sort_build_items(&arr, SORT_BY_MARKET_VALUE, i_items, 5, &cnt);

    merge_sort_items(m_items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, NULL);
    quick_sort_items(q_items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, NULL);
    insertion_sort_items(i_items, cnt, SORT_BY_MARKET_VALUE, SORT_DESC, NULL);

    for (size_t k = 0; k < cnt; ++k) {
        assert(m_items[k].index == q_items[k].index);
        assert(q_items[k].index == i_items[k].index);
        if (k > 0) {
            assert(m_items[k - 1].numeric_key >= m_items[k].numeric_key);
        }
    }
    stock_array_destroy(&arr);

    /* 6. Adjacency List Graph BFS & DFS Tests */
    StockGraph g;
    graph_init(&g);
    graph_add_vertex(&g, "TCS", "Technology", NULL);
    graph_add_vertex(&g, "INFY", "Technology", NULL);
    graph_add_vertex(&g, "WIPRO", "Technology", NULL);
    graph_add_edge(&g, "TCS", "INFY", 0.82, "IT Peers");
    graph_add_edge(&g, "INFY", "WIPRO", 0.76, "IT Peers");
    assert(fabs(graph_get_correlation(&g, "TCS", "INFY") - 0.82) < 1e-6);

    size_t bfs_out[8], dfs_out[8];
    assert(graph_bfs_related(&g, "TCS", 0.70, bfs_out, 8) == 2);
    assert(graph_dfs_sector_cluster(&g, "TCS", dfs_out, 8) == 3);
    graph_clear(&g);

    printf("[PASS] All Linked List, Stack, Queue, Min-Heap, Sorting & Graph tests passed!\n");
    return 0;
}
