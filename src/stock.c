#include "stock.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void stock_copy_str(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

int stock_stricmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    while (*a && *b) {
        int ca = toupper((unsigned char)*a);
        int cb = toupper((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return toupper((unsigned char)*a) - toupper((unsigned char)*b);
}

int stock_stristr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    if (!*needle) return 1;
    size_t nlen = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i] && toupper((unsigned char)p[i]) == toupper((unsigned char)needle[i])) {
            i++;
        }
        if (i == nlen) return 1;
    }
    return 0;
}

int stock_validate_ticker(const char *ticker) {
    if (!ticker) return 0;
    size_t len = 0;
    for (const char *p = ticker; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (!(isalnum(c) || c == '.' || c == '-' || c == '_')) {
            return 0;
        }
        len++;
    }
    return (len >= 1 && len < MAX_TICKER_LEN);
}

void stock_normalize_ticker(const char *raw, char *out, size_t out_size) {
    if (!raw || !out || out_size == 0) return;
    while (*raw && isspace((unsigned char)*raw)) raw++;
    size_t len = strlen(raw);
    while (len > 0 && isspace((unsigned char)raw[len - 1])) len--;
    if (len >= out_size) len = out_size - 1;
    for (size_t i = 0; i < len; ++i) {
        out[i] = (char)toupper((unsigned char)raw[i]);
    }
    out[len] = '\0';
}

void stock_recalculate_metrics(Stock *s) {
    if (!s) return;
    if (s->quantity <= 0) {
        s->quantity = 0;
        s->average_cost = 0.0;
        s->total_investment = 0.0;
        s->market_value = 0.0;
        s->unrealized_pl = 0.0;
    } else {
        s->total_investment = s->average_cost * (double)s->quantity;
        s->market_value = s->current_price * (double)s->quantity;
        s->unrealized_pl = s->market_value - s->total_investment;
    }
}

int stock_array_init(StockArray *arr, size_t initial_capacity) {
    if (!arr) return 0;
    arr->count = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 16;
    arr->items = (Stock *)calloc(arr->capacity, sizeof(Stock));
    if (!arr->items) {
        arr->capacity = 0;
        return 0;
    }
    return 1;
}

void stock_array_destroy(StockArray *arr) {
    if (!arr) return;
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

int stock_array_reserve(StockArray *arr, size_t min_capacity) {
    if (!arr) return 0;
    if (arr->capacity >= min_capacity) return 1;
    size_t new_cap = arr->capacity ? arr->capacity : 16;
    while (new_cap < min_capacity) {
        new_cap *= 2;
    }
    Stock *new_items = (Stock *)realloc(arr->items, new_cap * sizeof(Stock));
    if (!new_items) return 0;
    memset(new_items + arr->capacity, 0, (new_cap - arr->capacity) * sizeof(Stock));
    arr->items = new_items;
    arr->capacity = new_cap;
    return 1;
}

int stock_array_push(StockArray *arr, const Stock *stock, size_t *out_index) {
    if (!arr || !stock) return 0;
    if (arr->count >= arr->capacity) {
        if (!stock_array_reserve(arr, arr->count + 1)) return 0;
    }
    size_t idx = arr->count++;
    arr->items[idx] = *stock;
    stock_recalculate_metrics(&arr->items[idx]);
    if (out_index) *out_index = idx;
    return 1;
}

int stock_array_remove_at(StockArray *arr, size_t index) {
    if (!arr || index >= arr->count) return 0;
    if (index + 1 < arr->count) {
        memmove(&arr->items[index], &arr->items[index + 1],
                (arr->count - index - 1) * sizeof(Stock));
    }
    arr->count--;
    memset(&arr->items[arr->count], 0, sizeof(Stock));
    return 1;
}

Stock *stock_array_get(StockArray *arr, size_t index) {
    if (!arr || index >= arr->count) return NULL;
    return &arr->items[index];
}

const Stock *stock_array_get_const(const StockArray *arr, size_t index) {
    if (!arr || index >= arr->count) return NULL;
    return &arr->items[index];
}
