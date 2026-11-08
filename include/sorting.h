#ifndef SORTING_H
#define SORTING_H

#include "stock.h"
#include <stddef.h>

typedef enum {
    SORT_BY_MARKET_VALUE = 0,
    SORT_BY_PRICE        = 1,
    SORT_BY_QUANTITY     = 2,
    SORT_BY_PROFIT_LOSS  = 3,
    SORT_BY_INVESTMENT   = 4,
    SORT_BY_TICKER       = 5
} SortField;

typedef enum {
    SORT_ALG_MERGE     = 0,
    SORT_ALG_QUICK     = 1,
    SORT_ALG_INSERTION = 2
} SortAlgorithm;

typedef enum {
    SORT_DESC = 0,
    SORT_ASC  = 1
} SortOrder;

typedef struct {
    size_t index;
    double numeric_key;
    char ticker_key[24];
} SortItem;

typedef struct {
    SortAlgorithm algorithm;
    size_t element_count;
    size_t comparisons;
    size_t moves;
    double elapsed_microseconds;
} SortStats;

void sort_build_items(const StockArray *stocks, SortField field, SortItem *items_out, size_t max_items, size_t *count_out);
void merge_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out);
void quick_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out);
void insertion_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out);
void sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortAlgorithm alg, SortStats *stats_out);

#endif /* SORTING_H */
