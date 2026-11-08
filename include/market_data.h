#ifndef MARKET_DATA_H
#define MARKET_DATA_H

#include "portfolio.h"

/*
 * Market Data Adapter Module
 * --------------------------
 * Provides sample/manual price management by default and defines a clean
 * adapter interface so a live market data API (e.g., Alpha Vantage, Yahoo Finance,
 * Finnhub via WinHTTP) can be plugged in without modifying business logic or GUI code.
 * Sample prices are strictly labeled as simulated/manual and never misrepresented as live.
 */

typedef enum {
    MARKET_SOURCE_MANUAL_SAMPLE = 0,
    MARKET_SOURCE_CSV_IMPORT    = 1,
    MARKET_SOURCE_EXTERNAL_API  = 2
} MarketDataSourceType;

typedef struct {
    char ticker[24];
    double price;
    double previous_close;
    char source_label[32];
} MarketQuote;

const char *market_data_source_name(MarketDataSourceType source);
int market_data_apply_quote(Portfolio *p, const MarketQuote *quote);
size_t market_data_simulate_step(Portfolio *p, unsigned int *rng_seed);
int market_data_import_quotes_csv(Portfolio *p, const char *csv_path, size_t *updated_count_out);

#endif /* MARKET_DATA_H */
