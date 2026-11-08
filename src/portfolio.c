#include "portfolio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void portfolio_init(Portfolio *p) {
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->user.id = 1;
    stock_copy_str(p->user.username, sizeof(p->user.username), "AcademicTrader");
    stock_copy_str(p->user.email, sizeof(p->user.email), "investor@university.edu");
    stock_copy_str(p->user.currency, sizeof(p->user.currency), "INR");
    p->user.cash_balance = 1000000.0;
    transaction_make_timestamp(p->user.created_at, sizeof(p->user.created_at));

    stock_array_init(&p->stocks, 32);
    hashmap_init(&p->ticker_index);
    avl_init(&p->avl_index);
    dll_init(&p->transactions);
    stack_init(&p->undo_stack);
    queue_init(&p->pending_queue);
    heap_init(&p->price_alerts);
    graph_init(&p->correlation_graph);

    p->next_transaction_id = 1;
    p->next_stock_id = 1;
    stock_copy_str(p->db_path, sizeof(p->db_path), "data/portfolio.db");
}

void portfolio_destroy(Portfolio *p) {
    if (!p) return;
    stock_array_destroy(&p->stocks);
    hashmap_clear(&p->ticker_index);
    avl_clear(&p->avl_index);
    dll_clear(&p->transactions);
    stack_clear(&p->undo_stack);
    queue_clear(&p->pending_queue);
    heap_clear(&p->price_alerts);
    graph_clear(&p->correlation_graph);
}

static void sync_sector_graph_edges(Portfolio *p, size_t target_idx) {
    if (!p || target_idx >= p->stocks.count) return;
    const Stock *s = &p->stocks.items[target_idx];
    graph_add_vertex(&p->correlation_graph, s->ticker, s->sector, NULL);
    for (size_t i = 0; i < p->stocks.count; ++i) {
        if (i == target_idx) continue;
        const Stock *other = &p->stocks.items[i];
        if (stock_stricmp(s->sector, other->sector) == 0) {
            graph_add_edge(&p->correlation_graph, s->ticker, other->ticker, 0.72, "Same Sector Peer");
        } else {
            graph_add_edge(&p->correlation_graph, s->ticker, other->ticker, 0.22, "Cross-Sector Market");
        }
    }
}

void portfolio_rebuild_indices(Portfolio *p) {
    if (!p) return;
    hashmap_clear(&p->ticker_index);
    avl_clear(&p->avl_index);
    graph_clear(&p->correlation_graph);
    for (size_t i = 0; i < p->stocks.count; ++i) {
        Stock *s = &p->stocks.items[i];
        stock_recalculate_metrics(s);
        hashmap_put(&p->ticker_index, s->ticker, i);
        avl_insert(&p->avl_index, s->ticker, i, s->current_price);
        sync_sector_graph_edges(p, i);
    }
}

int portfolio_find_stock(const Portfolio *p, const char *ticker, size_t *index_out) {
    if (!p || !ticker || !*ticker) return 0;
    char norm[MAX_TICKER_LEN];
    stock_normalize_ticker(ticker, norm, sizeof(norm));
    return hashmap_get(&p->ticker_index, norm, index_out);
}

int portfolio_add_stock(Portfolio *p, const char *ticker, const char *name, const char *sector, double price) {
    if (!p || !ticker || !name || !*name || price <= 0.0) return 0;
    char norm[MAX_TICKER_LEN];
    stock_normalize_ticker(ticker, norm, sizeof(norm));
    if (!stock_validate_ticker(norm)) return 0;

    size_t existing_idx;
    if (portfolio_find_stock(p, norm, &existing_idx)) {
        Stock *s = &p->stocks.items[existing_idx];
        s->previous_price = s->current_price;
        s->current_price = price;
        stock_copy_str(s->name, sizeof(s->name), name);
        if (sector && *sector) {
            stock_copy_str(s->sector, sizeof(s->sector), sector);
        }
        stock_recalculate_metrics(s);
        avl_insert(&p->avl_index, s->ticker, existing_idx, s->current_price);
        heap_update_stock_price(&p->price_alerts, s->ticker, s->current_price);
        sync_sector_graph_edges(p, existing_idx);
        return 2; /* Updated existing stock */
    }

    Stock item;
    memset(&item, 0, sizeof(item));
    item.id = p->next_stock_id++;
    stock_copy_str(item.ticker, sizeof(item.ticker), norm);
    stock_copy_str(item.name, sizeof(item.name), name);
    stock_copy_str(item.sector, sizeof(item.sector), (sector && *sector) ? sector : "Technology");
    item.current_price = price;
    item.previous_price = price;
    item.in_watchlist = 1;
    stock_recalculate_metrics(&item);

    size_t new_idx = 0;
    if (!stock_array_push(&p->stocks, &item, &new_idx)) return 0;
    if (!hashmap_put(&p->ticker_index, item.ticker, new_idx)) {
        stock_array_remove_at(&p->stocks, new_idx);
        return 0;
    }
    avl_insert(&p->avl_index, item.ticker, new_idx, item.current_price);
    sync_sector_graph_edges(p, new_idx);
    return 1;
}

int portfolio_remove_stock(Portfolio *p, const char *ticker) {
    if (!p || !ticker || !*ticker) return 0;
    size_t idx;
    if (!portfolio_find_stock(p, ticker, &idx)) return 0;
    /* Prevent removing a stock from catalog if shares are actively owned */
    if (p->stocks.items[idx].quantity > 0) return 0;
    if (!stock_array_remove_at(&p->stocks, idx)) return 0;
    portfolio_rebuild_indices(p);
    return 1;
}

int portfolio_update_price(Portfolio *p, const char *ticker, double new_price) {
    if (!p || !ticker || new_price <= 0.0) return 0;
    size_t idx;
    if (!portfolio_find_stock(p, ticker, &idx)) return 0;
    Stock *s = &p->stocks.items[idx];
    s->previous_price = s->current_price;
    s->current_price = new_price;
    stock_recalculate_metrics(s);
    avl_insert(&p->avl_index, s->ticker, idx, new_price);
    heap_update_stock_price(&p->price_alerts, s->ticker, new_price);
    return 1;
}

int portfolio_set_watchlist(Portfolio *p, const char *ticker, int in_watchlist) {
    if (!p || !ticker) return 0;
    size_t idx;
    if (!portfolio_find_stock(p, ticker, &idx)) return 0;
    p->stocks.items[idx].in_watchlist = in_watchlist ? 1 : 0;
    return 1;
}

int portfolio_trade(Portfolio *p, const char *ticker, int quantity, double price, int is_buy) {
    if (!p || !ticker || quantity <= 0 || price <= 0.0) return 0;
    size_t idx;
    if (!portfolio_find_stock(p, ticker, &idx)) return 0;
    Stock *s = &p->stocks.items[idx];

    /* Prevent selling more shares than owned */
    if (!is_buy && quantity > s->quantity) return 0;

    UndoRecord undo;
    memset(&undo, 0, sizeof(undo));
    undo.transaction_id = p->next_transaction_id;
    undo.stock_id = s->id;
    stock_copy_str(undo.ticker, sizeof(undo.ticker), s->ticker);
    stock_copy_str(undo.type, sizeof(undo.type), is_buy ? "BUY" : "SELL");
    undo.quantity_delta = is_buy ? quantity : -quantity;
    undo.trade_price = price;
    undo.previous_quantity = s->quantity;
    undo.previous_average_cost = s->average_cost;
    undo.previous_realized_pl = s->realized_pl;

    double realized_delta = 0.0;
    if (is_buy) {
        double old_total_cost = s->average_cost * (double)s->quantity;
        s->quantity += quantity;
        s->average_cost = (old_total_cost + price * (double)quantity) / (double)s->quantity;
    } else {
        realized_delta = (price - s->average_cost) * (double)quantity;
        s->realized_pl += realized_delta;
        s->quantity -= quantity;
        if (s->quantity == 0) {
            s->average_cost = 0.0;
        }
    }
    undo.realized_pl_delta = realized_delta;
    stock_recalculate_metrics(s);

    if (!stack_push(&p->undo_stack, &undo)) {
        /* Roll back in-memory state if undo stack allocation fails */
        s->quantity = undo.previous_quantity;
        s->average_cost = undo.previous_average_cost;
        s->realized_pl = undo.previous_realized_pl;
        stock_recalculate_metrics(s);
        return 0;
    }

    if (!transaction_record(&p->transactions, p->next_transaction_id, p->user.id, s->id,
                            s->ticker, is_buy, quantity, price, realized_delta, NULL)) {
        UndoRecord discarded;
        stack_pop(&p->undo_stack, &discarded);
        s->quantity = undo.previous_quantity;
        s->average_cost = undo.previous_average_cost;
        s->realized_pl = undo.previous_realized_pl;
        stock_recalculate_metrics(s);
        return 0;
    }

    p->next_transaction_id++;
    return 1;
}

int portfolio_undo(Portfolio *p) {
    if (!p) return 0;
    UndoRecord u;
    if (!stack_pop(&p->undo_stack, &u)) return 0;

    size_t idx;
    if (!portfolio_find_stock(p, u.ticker, &idx)) return 0;
    Stock *s = &p->stocks.items[idx];

    s->quantity = u.previous_quantity;
    s->average_cost = u.previous_average_cost;
    s->realized_pl = u.previous_realized_pl;
    stock_recalculate_metrics(s);

    transaction_remove_last_by_id(&p->transactions, u.transaction_id);
    if (p->next_transaction_id > 1 && u.transaction_id + 1 == p->next_transaction_id) {
        p->next_transaction_id--;
    }
    return 1;
}

int portfolio_enqueue_task(Portfolio *p, TaskType type, const char *ticker, int quantity, double price) {
    if (!p || !ticker || !*ticker) return 0;
    char ts[32];
    transaction_make_timestamp(ts, sizeof(ts));
    return queue_enqueue(&p->pending_queue, type, ticker, quantity, price, ts);
}

size_t portfolio_process_pending_queue(Portfolio *p, size_t max_tasks) {
    if (!p) return 0;
    size_t processed = 0;
    PortfolioTask task;
    while ((max_tasks == 0 || processed < max_tasks) && queue_dequeue(&p->pending_queue, &task)) {
        switch (task.type) {
            case TASK_BUY_ORDER:
                if (portfolio_trade(p, task.ticker, task.quantity, task.price, 1)) processed++;
                break;
            case TASK_SELL_ORDER:
                if (portfolio_trade(p, task.ticker, task.quantity, task.price, 0)) processed++;
                break;
            case TASK_PRICE_UPDATE:
                if (portfolio_update_price(p, task.ticker, task.price)) processed++;
                break;
            case TASK_PORTFOLIO_SNAPSHOT:
                if (portfolio_record_snapshot(p, NULL)) processed++;
                break;
        }
    }
    return processed;
}

int portfolio_add_alert(Portfolio *p, const char *ticker, double target_price, AlertCondition cond, const char *notes) {
    if (!p || !ticker || target_price <= 0.0) return 0;
    size_t idx;
    if (!portfolio_find_stock(p, ticker, &idx)) return 0;
    const Stock *s = &p->stocks.items[idx];

    PriceAlert alert;
    memset(&alert, 0, sizeof(alert));
    stock_copy_str(alert.ticker, sizeof(alert.ticker), s->ticker);
    alert.target_price = target_price;
    alert.current_price = s->current_price;
    alert.condition = cond;
    stock_copy_str(alert.notes, sizeof(alert.notes),
                   (notes && *notes) ? notes : (cond == ALERT_ABOVE ? "Breakout target" : "Support threshold"));
    return heap_push(&p->price_alerts, &alert);
}

int portfolio_remove_alert(Portfolio *p, int alert_id) {
    if (!p) return 0;
    return heap_remove_by_id(&p->price_alerts, alert_id);
}

double portfolio_market_value(const Portfolio *p) {
    if (!p) return 0.0;
    double total = 0.0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        total += p->stocks.items[i].current_price * (double)p->stocks.items[i].quantity;
    }
    return total;
}

double portfolio_invested_value(const Portfolio *p) {
    if (!p) return 0.0;
    double total = 0.0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        total += p->stocks.items[i].average_cost * (double)p->stocks.items[i].quantity;
    }
    return total;
}

double portfolio_unrealized_pl(const Portfolio *p) {
    return portfolio_market_value(p) - portfolio_invested_value(p);
}

double portfolio_realized_pl(const Portfolio *p) {
    if (!p) return 0.0;
    double total = 0.0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        total += p->stocks.items[i].realized_pl;
    }
    return total;
}

size_t portfolio_active_holdings_count(const Portfolio *p) {
    if (!p) return 0;
    size_t active = 0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        if (p->stocks.items[i].quantity > 0) active++;
    }
    return active;
}

int portfolio_record_snapshot(Portfolio *p, const char *custom_timestamp) {
    if (!p) return 0;
    if (p->snapshot_count >= MAX_SNAPSHOTS) {
        memmove(&p->snapshots[0], &p->snapshots[1], (MAX_SNAPSHOTS - 1) * sizeof(PortfolioSnapshot));
        p->snapshot_count = MAX_SNAPSHOTS - 1;
    }
    PortfolioSnapshot *snap = &p->snapshots[p->snapshot_count++];
    snap->id = (int)p->snapshot_count;
    if (custom_timestamp && *custom_timestamp) {
        stock_copy_str(snap->timestamp, sizeof(snap->timestamp), custom_timestamp);
    } else {
        transaction_make_timestamp(snap->timestamp, sizeof(snap->timestamp));
    }
    snap->total_invested = portfolio_invested_value(p);
    snap->total_market_value = portfolio_market_value(p);
    snap->unrealized_pl = snap->total_market_value - snap->total_invested;
    snap->realized_pl = portfolio_realized_pl(p);
    return 1;
}

void portfolio_compute_analytics(const Portfolio *p, PortfolioAnalytics *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!p) return;

    out->total_market_value = portfolio_market_value(p);
    out->total_invested = portfolio_invested_value(p);
    out->total_unrealized_pl = out->total_market_value - out->total_invested;
    out->total_realized_pl = portfolio_realized_pl(p);
    out->net_total_pl = out->total_unrealized_pl + out->total_realized_pl;
    out->overall_return_pct = (out->total_invested > 0.0)
        ? (out->total_unrealized_pl / out->total_invested) * 100.0
        : 0.0;
    out->catalog_stocks_count = p->stocks.count;
    out->best_stock_index = -1;
    out->worst_stock_index = -1;

    double best_ret = -1e18;
    double worst_ret = 1e18;
    double hhi_sum = 0.0;

    for (size_t i = 0; i < p->stocks.count; ++i) {
        const Stock *s = &p->stocks.items[i];
        if (s->in_watchlist) out->watchlist_count++;
        if (s->quantity <= 0) continue;

        out->active_holdings_count++;
        double mv = s->current_price * (double)s->quantity;
        double inv = s->average_cost * (double)s->quantity;
        double upl = mv - inv;
        double ret_pct = (inv > 0.0) ? (upl / inv) * 100.0 : 0.0;

        if (ret_pct > best_ret) {
            best_ret = ret_pct;
            out->best_stock_index = (int)i;
        }
        if (ret_pct < worst_ret) {
            worst_ret = ret_pct;
            out->worst_stock_index = (int)i;
        }

        if (out->total_market_value > 0.0) {
            double w = mv / out->total_market_value;
            hhi_sum += w * w;
        }

        /* Group into sector allocation */
        size_t sec_idx = out->sector_count;
        for (size_t k = 0; k < out->sector_count; ++k) {
            if (stock_stricmp(out->sectors[k].sector, s->sector) == 0) {
                sec_idx = k;
                break;
            }
        }
        if (sec_idx == out->sector_count && out->sector_count < MAX_SECTORS) {
            sec_idx = out->sector_count++;
            stock_copy_str(out->sectors[sec_idx].sector,
                           sizeof(out->sectors[sec_idx].sector),
                           s->sector[0] ? s->sector : "General");
        }
        if (sec_idx < out->sector_count) {
            out->sectors[sec_idx].invested_value += inv;
            out->sectors[sec_idx].market_value += mv;
            out->sectors[sec_idx].unrealized_pl += upl;
            out->sectors[sec_idx].stock_count++;
        }
    }

    for (size_t k = 0; k < out->sector_count; ++k) {
        out->sectors[k].weight_percent = (out->total_market_value > 0.0)
            ? (out->sectors[k].market_value / out->total_market_value) * 100.0
            : 0.0;
    }

    /* Compute Graph pairwise correlation across active holdings */
    double corr_sum = 0.0;
    size_t pair_count = 0;
    for (size_t i = 0; i < p->stocks.count; ++i) {
        if (p->stocks.items[i].quantity <= 0) continue;
        for (size_t j = i + 1; j < p->stocks.count; ++j) {
            if (p->stocks.items[j].quantity <= 0) continue;
            corr_sum += graph_get_correlation(&p->correlation_graph,
                                              p->stocks.items[i].ticker,
                                              p->stocks.items[j].ticker);
            pair_count++;
        }
    }
    out->avg_pair_correlation = (pair_count > 0) ? (corr_sum / (double)pair_count) : 0.0;

    if (out->active_holdings_count == 0) {
        out->diversification_score = 0.0;
    } else if (out->active_holdings_count == 1) {
        out->diversification_score = 15.0;
    } else {
        /* Combine Herfindahl dispersion (1 - HHI) and low graph correlation bonus */
        double dispersion = (1.0 - hhi_sum) * 100.0;
        double corr_penalty = out->avg_pair_correlation * 20.0;
        double score = dispersion - corr_penalty + (double)(out->sector_count * 4);
        if (score < 5.0) score = 5.0;
        if (score > 100.0) score = 100.0;
        out->diversification_score = score;
    }
}

int portfolio_verify_integrity(const Portfolio *p, char *report_buf, size_t buf_size) {
    if (!p) return 0;
    int ok = 1;
    size_t index_mismatches = 0;
    int avl_balanced = avl_is_balanced(&p->avl_index);
    int heap_valid = heap_is_valid(&p->price_alerts);

    for (size_t i = 0; i < p->stocks.count; ++i) {
        size_t found_idx = (size_t)-1;
        if (!hashmap_get(&p->ticker_index, p->stocks.items[i].ticker, &found_idx) || found_idx != i) {
            index_mismatches++;
            ok = 0;
        }
        if (!avl_search(&p->avl_index, p->stocks.items[i].ticker, &found_idx) || found_idx != i) {
            index_mismatches++;
            ok = 0;
        }
    }
    if (!avl_balanced || !heap_valid) ok = 0;

    if (report_buf && buf_size > 0) {
        snprintf(report_buf, buf_size,
                 "DSA & Portfolio Integrity: %s (Mismatches=%zu) | Stocks=%zu | HashSize=%zu (Collisions=%zu) | "
                 "AVLSize=%zu (Height=%d, Balanced=%s) | HeapAlerts=%zu (Valid=%s) | TxCount=%zu | UndoDepth=%zu",
                 ok ? "PASSED" : "FAILED",
                 index_mismatches,
                 p->stocks.count,
                 p->ticker_index.size, p->ticker_index.collision_count,
                 p->avl_index.size, avl_height(&p->avl_index), avl_balanced ? "YES" : "NO",
                 p->price_alerts.size, heap_valid ? "YES" : "NO",
                 p->transactions.size,
                 p->undo_stack.size);
    }
    return ok;
}

void portfolio_seed_sample_data(Portfolio *p) {
    if (!p || p->stocks.count > 0) return;

    /* Add representative stocks across diverse sectors (Sample Prices) */
    portfolio_add_stock(p, "RELIANCE", "Reliance Industries Ltd", "Energy", 1450.00);
    portfolio_add_stock(p, "TCS", "Tata Consultancy Services", "Technology", 3200.00);
    portfolio_add_stock(p, "INFY", "Infosys Limited", "Technology", 1550.00);
    portfolio_add_stock(p, "HDFCBANK", "HDFC Bank Limited", "Financials", 1680.00);
    portfolio_add_stock(p, "ICICIBANK", "ICICI Bank Limited", "Financials", 1240.00);
    portfolio_add_stock(p, "ITC", "ITC Limited", "Consumer Goods", 430.00);
    portfolio_add_stock(p, "SUNPHARMA", "Sun Pharmaceutical Ind", "Healthcare", 1790.00);
    portfolio_add_stock(p, "LT", "Larsen & Toubro Limited", "Industrials", 3580.00);

    /* Record initial sample trades and price updates so Dashboard & Analytics are rich on first launch */
    portfolio_trade(p, "RELIANCE", 25, 1380.00, 1);
    portfolio_update_price(p, "RELIANCE", 1450.00);

    portfolio_trade(p, "TCS", 15, 3100.00, 1);
    portfolio_update_price(p, "TCS", 3200.00);

    portfolio_trade(p, "INFY", 40, 1490.00, 1);
    portfolio_trade(p, "INFY", 10, 1560.00, 0); /* Realizes +700.00 P/L */
    portfolio_update_price(p, "INFY", 1550.00);

    portfolio_trade(p, "HDFCBANK", 30, 1620.00, 1);
    portfolio_update_price(p, "HDFCBANK", 1680.00);

    portfolio_trade(p, "ITC", 60, 445.00, 1);
    portfolio_update_price(p, "ITC", 430.00);

    portfolio_trade(p, "SUNPHARMA", 12, 1710.00, 1);
    portfolio_update_price(p, "SUNPHARMA", 1790.00);

    /* Configure sample Min-Heap Price Alerts */
    portfolio_add_alert(p, "RELIANCE", 1500.00, ALERT_ABOVE, "Breakout above 1500");
    portfolio_add_alert(p, "TCS", 3250.00, ALERT_ABOVE, "Target resistance");
    portfolio_add_alert(p, "ITC", 425.00, ALERT_BELOW, "Accumulate support zone");
    portfolio_add_alert(p, "HDFCBANK", 1675.00, ALERT_ABOVE, "Already triggered alert");

    /* Seed historical portfolio snapshots for the performance chart */
    if (p->snapshot_count == 0) {
        double cur_inv = portfolio_invested_value(p);
        double cur_mv  = portfolio_market_value(p);
        double cur_rpl = portfolio_realized_pl(p);
        const char *dates[6] = {
            "2026-11-03 15:30:00",
            "2026-11-04 15:30:00",
            "2026-11-05 15:30:00",
            "2026-11-06 15:30:00",
            "2026-11-07 15:30:00",
            "2026-11-08 09:30:00"
        };
        const double factors[6] = { 0.965, 0.978, 0.971, 0.989, 0.994, 1.000 };
        for (int i = 0; i < 6; ++i) {
            PortfolioSnapshot *s = &p->snapshots[p->snapshot_count++];
            s->id = i + 1;
            stock_copy_str(s->timestamp, sizeof(s->timestamp), dates[i]);
            s->total_invested = cur_inv;
            s->total_market_value = floor(cur_mv * factors[i] * 100.0 + 0.5) / 100.0;
            s->unrealized_pl = s->total_market_value - s->total_invested;
            s->realized_pl = (i >= 3) ? cur_rpl : 0.0;
        }
    }
}
