#include "sorting.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/*
 * Sorting Algorithms Module
 * Implements:
 *   1. Merge Sort     - O(n log n) worst-case, stable
 *   2. Quick Sort     - O(n log n) average with median-of-three pivot selection
 *   3. Insertion Sort - O(n^2) worst-case, O(n) adaptive on nearly-sorted arrays
 */

static double now_microseconds(void) {
    return ((double)clock() * 1000000.0) / (double)CLOCKS_PER_SEC;
}

static int compare_sort_items(const SortItem *a, const SortItem *b, SortField field,
                              SortOrder order, size_t *comparisons) {
    if (comparisons) (*comparisons)++;
    int cmp = 0;
    if (field == SORT_BY_TICKER) {
        cmp = stock_stricmp(a->ticker_key, b->ticker_key);
    } else {
        if (fabs(a->numeric_key - b->numeric_key) > 1e-9) {
            cmp = (a->numeric_key > b->numeric_key) ? 1 : -1;
        } else {
            cmp = stock_stricmp(b->ticker_key, a->ticker_key);
        }
    }
    return (order == SORT_ASC) ? cmp : -cmp;
}

void sort_build_items(const StockArray *stocks, SortField field, SortItem *items_out,
                      size_t max_items, size_t *count_out) {
    if (!stocks || !items_out) {
        if (count_out) *count_out = 0;
        return;
    }
    size_t n = (stocks->count < max_items) ? stocks->count : max_items;
    for (size_t i = 0; i < n; ++i) {
        const Stock *s = &stocks->items[i];
        items_out[i].index = i;
        stock_copy_str(items_out[i].ticker_key, sizeof(items_out[i].ticker_key), s->ticker);
        switch (field) {
            case SORT_BY_PRICE:
                items_out[i].numeric_key = s->current_price;
                break;
            case SORT_BY_QUANTITY:
                items_out[i].numeric_key = (double)s->quantity;
                break;
            case SORT_BY_PROFIT_LOSS:
                items_out[i].numeric_key = (s->current_price - s->average_cost) * (double)s->quantity;
                break;
            case SORT_BY_INVESTMENT:
                items_out[i].numeric_key = s->average_cost * (double)s->quantity;
                break;
            case SORT_BY_TICKER:
                items_out[i].numeric_key = 0.0;
                break;
            case SORT_BY_MARKET_VALUE:
            default:
                items_out[i].numeric_key = s->current_price * (double)s->quantity;
                break;
        }
    }
    if (count_out) *count_out = n;
}

/* 1. Merge Sort */
static void merge_step(SortItem *a, SortItem *tmp, size_t left, size_t mid, size_t right,
                       SortField field, SortOrder order, size_t *comparisons, size_t *moves) {
    size_t i = left, j = mid, k = left;
    while (i < mid && j < right) {
        if (compare_sort_items(&a[i], &a[j], field, order, comparisons) <= 0) {
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
        }
        if (moves) (*moves)++;
    }
    while (i < mid) {
        tmp[k++] = a[i++];
        if (moves) (*moves)++;
    }
    while (j < right) {
        tmp[k++] = a[j++];
        if (moves) (*moves)++;
    }
    for (i = left; i < right; ++i) {
        a[i] = tmp[i];
    }
}

static void merge_sort_rec(SortItem *a, SortItem *tmp, size_t left, size_t right,
                           SortField field, SortOrder order, size_t *comparisons, size_t *moves) {
    if (right - left < 2) return;
    size_t mid = left + (right - left) / 2;
    merge_sort_rec(a, tmp, left, mid, field, order, comparisons, moves);
    merge_sort_rec(a, tmp, mid, right, field, order, comparisons, moves);
    merge_step(a, tmp, left, mid, right, field, order, comparisons, moves);
}

void merge_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out) {
    size_t comps = 0, moves = 0;
    double t0 = now_microseconds();
    if (items && count >= 2) {
        SortItem *tmp = (SortItem *)malloc(count * sizeof(SortItem));
        if (tmp) {
            merge_sort_rec(items, tmp, 0, count, field, order, &comps, &moves);
            free(tmp);
        }
    }
    double t1 = now_microseconds();
    if (stats_out) {
        stats_out->algorithm = SORT_ALG_MERGE;
        stats_out->element_count = count;
        stats_out->comparisons = comps;
        stats_out->moves = moves;
        stats_out->elapsed_microseconds = (t1 >= t0) ? (t1 - t0) : 0.0;
    }
}

/* 2. Quick Sort (Median-of-Three Pivot) */
static void swap_item(SortItem *a, SortItem *b, size_t *moves) {
    SortItem t = *a;
    *a = *b;
    *b = t;
    if (moves) (*moves) += 3;
}

static void quick_sort_rec(SortItem *a, ptrdiff_t low, ptrdiff_t high,
                           SortField field, SortOrder order, size_t *comparisons, size_t *moves) {
    if (low >= high) return;

    ptrdiff_t mid = low + (high - low) / 2;
    if (compare_sort_items(&a[mid], &a[low], field, order, comparisons) < 0)
        swap_item(&a[low], &a[mid], moves);
    if (compare_sort_items(&a[high], &a[low], field, order, comparisons) < 0)
        swap_item(&a[low], &a[high], moves);
    if (compare_sort_items(&a[high], &a[mid], field, order, comparisons) < 0)
        swap_item(&a[mid], &a[high], moves);

    SortItem pivot = a[mid];
    ptrdiff_t i = low;
    ptrdiff_t j = high;
    while (i <= j) {
        while (compare_sort_items(&a[i], &pivot, field, order, comparisons) < 0) i++;
        while (compare_sort_items(&a[j], &pivot, field, order, comparisons) > 0) j--;
        if (i <= j) {
            swap_item(&a[i], &a[j], moves);
            i++;
            j--;
        }
    }
    if (low < j) quick_sort_rec(a, low, j, field, order, comparisons, moves);
    if (i < high) quick_sort_rec(a, i, high, field, order, comparisons, moves);
}

void quick_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out) {
    size_t comps = 0, moves = 0;
    double t0 = now_microseconds();
    if (items && count >= 2) {
        quick_sort_rec(items, 0, (ptrdiff_t)count - 1, field, order, &comps, &moves);
    }
    double t1 = now_microseconds();
    if (stats_out) {
        stats_out->algorithm = SORT_ALG_QUICK;
        stats_out->element_count = count;
        stats_out->comparisons = comps;
        stats_out->moves = moves;
        stats_out->elapsed_microseconds = (t1 >= t0) ? (t1 - t0) : 0.0;
    }
}

/* 3. Insertion Sort */
void insertion_sort_items(SortItem *items, size_t count, SortField field, SortOrder order, SortStats *stats_out) {
    size_t comps = 0, moves = 0;
    double t0 = now_microseconds();
    if (items && count >= 2) {
        for (size_t i = 1; i < count; ++i) {
            SortItem key = items[i];
            size_t j = i;
            while (j > 0 && compare_sort_items(&items[j - 1], &key, field, order, &comps) > 0) {
                items[j] = items[j - 1];
                moves++;
                j--;
            }
            items[j] = key;
            moves++;
        }
    }
    double t1 = now_microseconds();
    if (stats_out) {
        stats_out->algorithm = SORT_ALG_INSERTION;
        stats_out->element_count = count;
        stats_out->comparisons = comps;
        stats_out->moves = moves;
        stats_out->elapsed_microseconds = (t1 >= t0) ? (t1 - t0) : 0.0;
    }
}

void sort_items(SortItem *items, size_t count, SortField field, SortOrder order,
                SortAlgorithm alg, SortStats *stats_out) {
    switch (alg) {
        case SORT_ALG_QUICK:
            quick_sort_items(items, count, field, order, stats_out);
            break;
        case SORT_ALG_INSERTION:
            insertion_sort_items(items, count, field, order, stats_out);
            break;
        case SORT_ALG_MERGE:
        default:
            merge_sort_items(items, count, field, order, stats_out);
            break;
    }
}
