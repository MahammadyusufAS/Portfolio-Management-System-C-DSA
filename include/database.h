#ifndef DATABASE_H
#define DATABASE_H

#include "portfolio.h"
#include "sqlite3_api.h"
#include <stddef.h>

/*
 * SQLite Database Persistence Layer
 * Tables managed:
 *   1. users
 *   2. stocks
 *   3. holdings
 *   4. transactions
 *   5. watchlist
 *   6. price_alerts
 *   7. portfolio_snapshots
 */

int db_init(const char *db_path);
int db_save_portfolio(const Portfolio *p, const char *db_path);
int db_load_portfolio(Portfolio *p, const char *db_path);
int db_record_trade_atomic(const Portfolio *p, const Stock *updated_stock,
                           const TransactionNode *tx, const char *db_path);
int db_backup(const char *source_db_path, const char *backup_db_path);
int db_restore(const char *backup_db_path, const char *target_db_path, Portfolio *p);
int db_integrity_check(const char *db_path, char *result_buf, size_t buf_size);
const char *db_last_error(void);

#endif /* DATABASE_H */
