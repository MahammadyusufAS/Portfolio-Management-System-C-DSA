#include "portfolio.h"
#include "database.h"
#include "market_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int main(void) {
    printf("=== Running Unit Tests: Portfolio Engine, Edge Cases & SQLite Persistence ===\n");

    Portfolio p;
    portfolio_init(&p);

    /* 1. Edge Case Validation: Invalid inputs must be rejected */
    assert(portfolio_add_stock(&p, "", "Empty Ticker", "Tech", 100.0) == 0);
    assert(portfolio_add_stock(&p, "BAD TICKER!", "Invalid Chars", "Tech", 100.0) == 0);
    assert(portfolio_add_stock(&p, "VALID", "", "Tech", 100.0) == 0);
    assert(portfolio_add_stock(&p, "VALID", "Negative Price", "Tech", -50.0) == 0);
    assert(portfolio_add_stock(&p, "VALID", "Zero Price", "Tech", 0.0) == 0);
    assert(p.stocks.count == 0);

    /*Empty portfolio metrics */
    assert(fabs(portfolio_market_value(&p)) < 1e-9);
    assert(fabs(portfolio_invested_value(&p)) < 1e-9);
    assert(portfolio_undo(&p) == 0);

    /* 2. Stock Addition, Duplicate Update & Weighted-Average Cost Calculation */
    assert(portfolio_add_stock(&p, "RELIANCE", "Reliance Industries", "Energy", 1400.0) == 1);
    assert(portfolio_add_stock(&p, "TCS", "Tata Consultancy Services", "Technology", 3000.0) == 1);
    assert(p.stocks.count == 2);

    /* Duplicate add updates price and returns 2 */
    assert(portfolio_add_stock(&p, "reliance", "Reliance Industries Ltd", "Energy", 1450.0) == 2);
    assert(p.stocks.count == 2);

    /* Buy 10 shares @ 1400, then Buy 10 shares @ 1600 -> Weighted Avg Cost = 1500.00 */
    assert(portfolio_trade(&p, "RELIANCE", 10, 1400.0, 1) == 1);
    assert(portfolio_trade(&p, "RELIANCE", 10, 1600.0, 1) == 1);

    size_t r_idx = 0;
    assert(portfolio_find_stock(&p, "RELIANCE", &r_idx) == 1);
    assert(p.stocks.items[r_idx].quantity == 20);
    assert(fabs(p.stocks.items[r_idx].average_cost - 1500.0) < 1e-6);

    /* Edge Case: Reject zero/negative quantity or selling more shares than owned */
    assert(portfolio_trade(&p, "RELIANCE", 0, 1500.0, 1) == 0);
    assert(portfolio_trade(&p, "RELIANCE", -5, 1500.0, 1) == 0);
    assert(portfolio_trade(&p, "RELIANCE", 25, 1700.0, 0) == 0); /* Only 20 owned! */
    assert(p.stocks.items[r_idx].quantity == 20);

    /* Cannot remove a stock while shares are owned */
    assert(portfolio_remove_stock(&p, "RELIANCE") == 0);

    /* 3. Partial Sell & Realized / Unrealized Profit/Loss Calculation */
    /* Sell 5 shares @ 1700 (Avg cost 1500) -> Realized P/L = (1700 - 1500) * 5 = +1000.00 */
    assert(portfolio_trade(&p, "RELIANCE", 5, 1700.0, 0) == 1);
    assert(p.stocks.items[r_idx].quantity == 15);
    assert(fabs(p.stocks.items[r_idx].average_cost - 1500.0) < 1e-6);
    assert(fabs(p.stocks.items[r_idx].realized_pl - 1000.0) < 1e-6);

    /* Update price to 1800 -> Unrealized P/L on 15 shares = (1800 - 1500) * 15 = +4500.00 */
    assert(portfolio_update_price(&p, "RELIANCE", 1800.0) == 1);
    assert(fabs(portfolio_unrealized_pl(&p) - 4500.0) < 1e-6);
    assert(fabs(portfolio_realized_pl(&p) - 1000.0) < 1e-6);

    /* 4. Undo Transaction Consistency */
    assert(p.transactions.size == 3);
    assert(portfolio_undo(&p) == 1); /* Undoes the SELL of 5 shares */
    assert(p.stocks.items[r_idx].quantity == 20);
    assert(fabs(p.stocks.items[r_idx].realized_pl - 0.0) < 1e-6);
    assert(p.transactions.size == 2);

    /* 5. SQLite Database Persistence, Backup, Restore & Integrity Verification */
    const char *test_db = "data/test_unit_portfolio.db";
    const char *backup_db = "data/test_unit_backup.db";
    remove(test_db);
    remove(backup_db);

    portfolio_add_alert(&p, "RELIANCE", 1900.0, ALERT_ABOVE, "Unit test alert");
    portfolio_record_snapshot(&p, "2026-11-08 10:00:00");

    assert(db_save_portfolio(&p, test_db) == 1);

    char db_rep[512];
    assert(db_integrity_check(test_db, db_rep, sizeof(db_rep)) == 1);
    printf("  %s\n", db_rep);

    assert(db_backup(test_db, backup_db) == 1);

    Portfolio loaded;
    portfolio_init(&loaded);
    assert(db_load_portfolio(&loaded, backup_db) == 1);
    assert(loaded.stocks.count == 2);
    assert(loaded.transactions.size == 2);
    assert(loaded.price_alerts.size == 1);
    assert(loaded.snapshot_count == 1);

    size_t l_idx = 0;
    assert(portfolio_find_stock(&loaded, "RELIANCE", &l_idx) == 1);
    assert(loaded.stocks.items[l_idx].quantity == 20);
    assert(fabs(loaded.stocks.items[l_idx].average_cost - 1500.0) < 1e-6);

    char dsa_rep[512];
    assert(portfolio_verify_integrity(&loaded, dsa_rep, sizeof(dsa_rep)) == 1);
    printf("  %s\n", dsa_rep);

    portfolio_destroy(&loaded);
    portfolio_destroy(&p);
    remove(test_db);
    remove(backup_db);

    printf("[PASS] All Portfolio Engine, Edge Case & SQLite Persistence tests passed!\n");
    return 0;
}
