#include "market_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

const char *market_data_source_name(MarketDataSourceType source) {
    switch (source) {
        case MARKET_SOURCE_CSV_IMPORT:
            return "CSV Price Feed Import";
        case MARKET_SOURCE_EXTERNAL_API:
            return "External Market API Adapter";
        case MARKET_SOURCE_MANUAL_SAMPLE:
        default:
            return "Manual / Simulated Sample Feed (Offline)";
    }
}

int market_data_apply_quote(Portfolio *p, const MarketQuote *quote) {
    if (!p || !quote || !quote->ticker[0] || quote->price <= 0.0) return 0;
    return portfolio_update_price(p, quote->ticker, quote->price);
}

size_t market_data_simulate_step(Portfolio *p, unsigned int *rng_seed) {
    if (!p || p->stocks.count == 0) return 0;
    unsigned int seed = (rng_seed && *rng_seed) ? *rng_seed : 1337u;
    size_t updated = 0;

    for (size_t i = 0; i < p->stocks.count; ++i) {
        Stock *s = &p->stocks.items[i];
        /* Deterministic LCG step for reproducible sample price movement (-2.5% to +2.7%) */
        seed = seed * 1664525u + 1013904223u;
        int delta_bp = (int)(seed % 521u) - 250; /* basis points */
        double factor = 1.0 + ((double)delta_bp / 10000.0);
        double new_price = floor((s->current_price * factor) * 100.0 + 0.5) / 100.0;
        if (new_price < 1.0) new_price = 1.0;
        if (portfolio_update_price(p, s->ticker, new_price)) {
            updated++;
        }
    }
    if (rng_seed) *rng_seed = seed;
    portfolio_record_snapshot(p, NULL);
    return updated;
}

int market_data_import_quotes_csv(Portfolio *p, const char *csv_path, size_t *updated_count_out) {
    if (updated_count_out) *updated_count_out = 0;
    if (!p || !csv_path || !*csv_path) return 0;
    FILE *f = fopen(csv_path, "r");
    if (!f) return 0;

    char line[256];
    size_t updated = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        char ticker[24];
        double price = 0.0;
        if (sscanf(line, "%23[^,],%lf", ticker, &price) == 2 && price > 0.0) {
            if (portfolio_update_price(p, ticker, price)) {
                updated++;
            }
        }
    }
    fclose(f);
    if (updated_count_out) *updated_count_out = updated;
    return 1;
}
