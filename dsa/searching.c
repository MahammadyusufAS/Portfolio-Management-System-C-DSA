#include "searching.h"
#include "sorting.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Searching Algorithms Module
 * Implements and compares:
 *   1. Linear Search       - O(n)
 *   2. Binary Search       - O(log n) over sorted index array
 *   3. Hash Table Search   - O(1) average via separate chaining
 *   4. AVL Tree Search     - O(log n) worst-case via balanced BST
 */

static double now_nanoseconds(void) {
    return ((double)clock() * 1000000000.0) / (double)CLOCKS_PER_SEC;
}

int search_linear_ticker(const StockArray *stocks, const char *ticker, size_t *index_out, size_t *steps_out) {
    size_t steps = 0;
    if (!stocks || !ticker || !*ticker) {
        if (steps_out) *steps_out = 0;
        return 0;
    }
    for (size_t i = 0; i < stocks->count; ++i) {
        steps++;
        if (stock_stricmp(stocks->items[i].ticker, ticker) == 0) {
            if (index_out) *index_out = i;
            if (steps_out) *steps_out = steps;
            return 1;
        }
    }
    if (steps_out) *steps_out = steps;
    return 0;
}

size_t search_linear_substring(const StockArray *stocks, const char *query, size_t *indices_out, size_t max_out) {
    if (!stocks || !indices_out || max_out == 0) return 0;
    size_t count = 0;
    for (size_t i = 0; i < stocks->count && count < max_out; ++i) {
        const Stock *s = &stocks->items[i];
        if (!query || !*query ||
            stock_stristr(s->ticker, query) ||
            stock_stristr(s->name, query) ||
            stock_stristr(s->sector, query)) {
            indices_out[count++] = i;
        }
    }
    return count;
}

void search_prepare_sorted_ticker_indices(const StockArray *stocks, size_t *sorted_indices_out) {
    if (!stocks || !sorted_indices_out || stocks->count == 0) return;
    SortItem *tmp = (SortItem *)malloc(stocks->count * sizeof(SortItem));
    if (!tmp) {
        for (size_t i = 0; i < stocks->count; ++i) sorted_indices_out[i] = i;
        return;
    }
    size_t count = 0;
    sort_build_items(stocks, SORT_BY_TICKER, tmp, stocks->count, &count);
    merge_sort_items(tmp, count, SORT_BY_TICKER, SORT_ASC, NULL);
    for (size_t i = 0; i < count; ++i) {
        sorted_indices_out[i] = tmp[i].index;
    }
    free(tmp);
}

int search_binary_ticker(const StockArray *stocks, const size_t *sorted_indices, size_t count,
                         const char *ticker, size_t *index_out, size_t *steps_out) {
    size_t steps = 0;
    if (!stocks || !sorted_indices || count == 0 || !ticker || !*ticker) {
        if (steps_out) *steps_out = 0;
        return 0;
    }
    size_t low = 0;
    size_t high = count;
    while (low < high) {
        steps++;
        size_t mid = low + (high - low) / 2;
        size_t idx = sorted_indices[mid];
        int cmp = stock_stricmp(stocks->items[idx].ticker, ticker);
        if (cmp == 0) {
            if (index_out) *index_out = idx;
            if (steps_out) *steps_out = steps;
            return 1;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    if (steps_out) *steps_out = steps;
    return 0;
}

int search_hash_ticker(const HashMap *map, const char *ticker, size_t *index_out, size_t *steps_out) {
    return hashmap_get_with_steps(map, ticker, index_out, steps_out);
}

int search_avl_ticker(const AVLTree *tree, const char *ticker, size_t *index_out, size_t *steps_out) {
    return avl_search_with_steps(tree, ticker, index_out, steps_out);
}

void search_compare_all(const StockArray *stocks, const HashMap *map, const AVLTree *tree,
                        const char *ticker, SearchComparisonReport *report_out) {
    if (!report_out) return;
    memset(report_out, 0, sizeof(*report_out));
    if (!ticker) return;
    stock_normalize_ticker(ticker, report_out->query_ticker, sizeof(report_out->query_ticker));

    /* Prepare sorted indices for Binary Search */
    size_t *sorted_idx = NULL;
    if (stocks && stocks->count > 0) {
        sorted_idx = (size_t *)malloc(stocks->count * sizeof(size_t));
        if (sorted_idx) {
            search_prepare_sorted_ticker_indices(stocks, sorted_idx);
        }
    }

    const int iterations = 200;
    double t0, t1;

    /* 1. Linear Search */
    t0 = now_nanoseconds();
    for (int r = 0; r < iterations; ++r) {
        report_out->linear_metric.found = search_linear_ticker(
            stocks, ticker, &report_out->linear_metric.index, &report_out->linear_metric.steps);
    }
    t1 = now_nanoseconds();
    report_out->linear_metric.elapsed_nanoseconds = (t1 - t0) / iterations;

    /* 2. Binary Search */
    t0 = now_nanoseconds();
    for (int r = 0; r < iterations; ++r) {
        report_out->binary_metric.found = search_binary_ticker(
            stocks, sorted_idx, stocks ? stocks->count : 0, ticker,
            &report_out->binary_metric.index, &report_out->binary_metric.steps);
    }
    t1 = now_nanoseconds();
    report_out->binary_metric.elapsed_nanoseconds = (t1 - t0) / iterations;

    /* 3. Hash Table Search */
    t0 = now_nanoseconds();
    for (int r = 0; r < iterations; ++r) {
        report_out->hash_metric.found = search_hash_ticker(
            map, ticker, &report_out->hash_metric.index, &report_out->hash_metric.steps);
    }
    t1 = now_nanoseconds();
    report_out->hash_metric.elapsed_nanoseconds = (t1 - t0) / iterations;

    /* 4. AVL Tree Search */
    t0 = now_nanoseconds();
    for (int r = 0; r < iterations; ++r) {
        report_out->avl_metric.found = search_avl_ticker(
            tree, ticker, &report_out->avl_metric.index, &report_out->avl_metric.steps);
    }
    t1 = now_nanoseconds();
    report_out->avl_metric.elapsed_nanoseconds = (t1 - t0) / iterations;

    free(sorted_idx);
}
