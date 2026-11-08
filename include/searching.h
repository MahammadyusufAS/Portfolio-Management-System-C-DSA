#ifndef SEARCHING_H
#define SEARCHING_H

#include "stock.h"
#include "hashmap.h"
#include "avl_tree.h"
#include <stddef.h>

typedef struct {
    int found;
    size_t index;
    size_t steps;
    double elapsed_nanoseconds;
} SearchMetric;

typedef struct {
    char query_ticker[24];
    SearchMetric linear_metric;
    SearchMetric binary_metric;
    SearchMetric hash_metric;
    SearchMetric avl_metric;
} SearchComparisonReport;

/* 1. Linear Search (exact ticker) - O(n) */
int search_linear_ticker(const StockArray *stocks, const char *ticker, size_t *index_out, size_t *steps_out);

/* 2. Linear Search (substring match on ticker, company name, or sector) - O(n) */
size_t search_linear_substring(const StockArray *stocks, const char *query, size_t *indices_out, size_t max_out);

/* 3. Binary Search (on ticker-sorted index array) - O(log n) */
void search_prepare_sorted_ticker_indices(const StockArray *stocks, size_t *sorted_indices_out);
int search_binary_ticker(const StockArray *stocks, const size_t *sorted_indices, size_t count,
                         const char *ticker, size_t *index_out, size_t *steps_out);

/* 4. Hash-based Search - O(1) average */
int search_hash_ticker(const HashMap *map, const char *ticker, size_t *index_out, size_t *steps_out);

/* 5. AVL Tree Search - O(log n) worst-case */
int search_avl_ticker(const AVLTree *tree, const char *ticker, size_t *index_out, size_t *steps_out);

/* Benchmark all 4 searching algorithms side-by-side */
void search_compare_all(const StockArray *stocks, const HashMap *map, const AVLTree *tree,
                        const char *ticker, SearchComparisonReport *report_out);

#endif /* SEARCHING_H */
