#ifndef STOCK_H
#define STOCK_H

#include <stddef.h>

#define MAX_TICKER_LEN 24
#define MAX_NAME_LEN   80
#define MAX_SECTOR_LEN 40

typedef struct {
    int id;
    char ticker[MAX_TICKER_LEN];
    char name[MAX_NAME_LEN];
    char sector[MAX_SECTOR_LEN];
    double current_price;
    double previous_price;
    int quantity;
    double average_cost;
    double total_investment;
    double market_value;
    double unrealized_pl;
    double realized_pl;
    int in_watchlist;
} Stock;

/* Dynamic Array for Portfolio Holdings & Stock Records */
typedef struct {
    Stock *items;
    size_t count;
    size_t capacity;
} StockArray;

void stock_copy_str(char *dst, size_t dst_size, const char *src);
int stock_stricmp(const char *a, const char *b);
int stock_stristr(const char *haystack, const char *needle);
int stock_validate_ticker(const char *ticker);
void stock_normalize_ticker(const char *raw, char *out, size_t out_size);
void stock_recalculate_metrics(Stock *s);

int stock_array_init(StockArray *arr, size_t initial_capacity);
void stock_array_destroy(StockArray *arr);
int stock_array_reserve(StockArray *arr, size_t min_capacity);
int stock_array_push(StockArray *arr, const Stock *stock, size_t *out_index);
int stock_array_remove_at(StockArray *arr, size_t index);
Stock *stock_array_get(StockArray *arr, size_t index);
const Stock *stock_array_get_const(const StockArray *arr, size_t index);

#endif /* STOCK_H */
