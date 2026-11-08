#include "database.h"
#include "sqlite3_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#else
#include <dlfcn.h>
#include <sys/stat.h>
#endif

/*
 * Dynamic/Static SQLite3 Binding + Complete Persistence Layer
 * Automatically binds to:
 *   - Windows: sqlite3.dll or C:\Windows\System32\winsqlite3.dll (built into Windows 10/11)
 *   - Linux/macOS: libsqlite3.so.0 / libsqlite3.so / libsqlite3.dylib
 * Manages all 7 schema tables with ACID transactions, prepared statements,
 * foreign keys, online backup API, and PRAGMA integrity_check.
 */

typedef int (*fn_sqlite3_open)(const char *, sqlite3 **);
typedef int (*fn_sqlite3_close)(sqlite3 *);
typedef int (*fn_sqlite3_exec)(sqlite3 *, const char *, int (*)(void*,int,char**,char**), void *, char **);
typedef void (*fn_sqlite3_free)(void *);
typedef const char *(*fn_sqlite3_errmsg)(sqlite3 *);
typedef sqlite3_int64 (*fn_sqlite3_last_insert_rowid)(sqlite3 *);
typedef int (*fn_sqlite3_prepare_v2)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
typedef int (*fn_sqlite3_step)(sqlite3_stmt *);
typedef int (*fn_sqlite3_reset)(sqlite3_stmt *);
typedef int (*fn_sqlite3_finalize)(sqlite3_stmt *);
typedef int (*fn_sqlite3_bind_int)(sqlite3_stmt *, int, int);
typedef int (*fn_sqlite3_bind_int64)(sqlite3_stmt *, int, sqlite3_int64);
typedef int (*fn_sqlite3_bind_double)(sqlite3_stmt *, int, double);
typedef int (*fn_sqlite3_bind_text)(sqlite3_stmt *, int, const char *, int, void(*)(void*));
typedef int (*fn_sqlite3_column_int)(sqlite3_stmt *, int);
typedef sqlite3_int64 (*fn_sqlite3_column_int64)(sqlite3_stmt *, int);
typedef double (*fn_sqlite3_column_double)(sqlite3_stmt *, int);
typedef const unsigned char *(*fn_sqlite3_column_text)(sqlite3_stmt *, int);
typedef sqlite3_backup *(*fn_sqlite3_backup_init)(sqlite3 *, const char *, sqlite3 *, const char *);
typedef int (*fn_sqlite3_backup_step)(sqlite3_backup *, int);
typedef int (*fn_sqlite3_backup_finish)(sqlite3_backup *);

static struct {
    int initialized;
    int available;
    const char *backend_name;
    fn_sqlite3_open open;
    fn_sqlite3_close close;
    fn_sqlite3_exec exec;
    fn_sqlite3_free free_fn;
    fn_sqlite3_errmsg errmsg;
    fn_sqlite3_last_insert_rowid last_rowid;
    fn_sqlite3_prepare_v2 prepare_v2;
    fn_sqlite3_step step;
    fn_sqlite3_reset reset;
    fn_sqlite3_finalize finalize;
    fn_sqlite3_bind_int bind_int;
    fn_sqlite3_bind_int64 bind_int64;
    fn_sqlite3_bind_double bind_double;
    fn_sqlite3_bind_text bind_text;
    fn_sqlite3_column_int column_int;
    fn_sqlite3_column_int64 column_int64;
    fn_sqlite3_column_double column_double;
    fn_sqlite3_column_text column_text;
    fn_sqlite3_backup_init backup_init;
    fn_sqlite3_backup_step backup_step;
    fn_sqlite3_backup_finish backup_finish;
} g_sql = {0};

static char g_last_db_error[512] = "No error";

static void set_db_error(const char *msg) {
    if (!msg) return;
    stock_copy_str(g_last_db_error, sizeof(g_last_db_error), msg);
}

const char *db_last_error(void) {
    return g_last_db_error;
}

#ifdef _WIN32
static void *load_sym(HMODULE h, const char *name) {
    return (void *)(uintptr_t)GetProcAddress(h, name);
}
#else
static void *load_sym(void *h, const char *name) {
    return dlsym(h, name);
}
#endif

int sp_sqlite_initialize(void) {
    if (g_sql.initialized) return g_sql.available;
    g_sql.initialized = 1;

#ifdef _WIN32
    HMODULE h = LoadLibraryA("sqlite3.dll");
    g_sql.backend_name = "sqlite3.dll";
    if (!h) {
        h = LoadLibraryA("winsqlite3.dll");
        g_sql.backend_name = "winsqlite3.dll (Windows System SQLite3)";
    }
    if (!h) {
        g_sql.backend_name = "Unavailable";
        return 0;
    }
#else
    void *h = dlopen("libsqlite3.so.0", RTLD_LAZY);
    g_sql.backend_name = "libsqlite3.so.0";
    if (!h) {
        h = dlopen("/usr/lib/x86_64-linux-gnu/libsqlite3.so.0", RTLD_LAZY);
    }
    if (!h) {
        h = dlopen("libsqlite3.so", RTLD_LAZY);
    }
    if (!h) {
        g_sql.backend_name = "Unavailable";
        return 0;
    }
#endif

    g_sql.open          = (fn_sqlite3_open)load_sym(h, "sqlite3_open");
    g_sql.close         = (fn_sqlite3_close)load_sym(h, "sqlite3_close");
    g_sql.exec          = (fn_sqlite3_exec)load_sym(h, "sqlite3_exec");
    g_sql.free_fn       = (fn_sqlite3_free)load_sym(h, "sqlite3_free");
    g_sql.errmsg        = (fn_sqlite3_errmsg)load_sym(h, "sqlite3_errmsg");
    g_sql.last_rowid    = (fn_sqlite3_last_insert_rowid)load_sym(h, "sqlite3_last_insert_rowid");
    g_sql.prepare_v2    = (fn_sqlite3_prepare_v2)load_sym(h, "sqlite3_prepare_v2");
    g_sql.step          = (fn_sqlite3_step)load_sym(h, "sqlite3_step");
    g_sql.reset         = (fn_sqlite3_reset)load_sym(h, "sqlite3_reset");
    g_sql.finalize      = (fn_sqlite3_finalize)load_sym(h, "sqlite3_finalize");
    g_sql.bind_int      = (fn_sqlite3_bind_int)load_sym(h, "sqlite3_bind_int");
    g_sql.bind_int64    = (fn_sqlite3_bind_int64)load_sym(h, "sqlite3_bind_int64");
    g_sql.bind_double   = (fn_sqlite3_bind_double)load_sym(h, "sqlite3_bind_double");
    g_sql.bind_text     = (fn_sqlite3_bind_text)load_sym(h, "sqlite3_bind_text");
    g_sql.column_int    = (fn_sqlite3_column_int)load_sym(h, "sqlite3_column_int");
    g_sql.column_int64  = (fn_sqlite3_column_int64)load_sym(h, "sqlite3_column_int64");
    g_sql.column_double = (fn_sqlite3_column_double)load_sym(h, "sqlite3_column_double");
    g_sql.column_text   = (fn_sqlite3_column_text)load_sym(h, "sqlite3_column_text");
    g_sql.backup_init   = (fn_sqlite3_backup_init)load_sym(h, "sqlite3_backup_init");
    g_sql.backup_step   = (fn_sqlite3_backup_step)load_sym(h, "sqlite3_backup_step");
    g_sql.backup_finish = (fn_sqlite3_backup_finish)load_sym(h, "sqlite3_backup_finish");

    g_sql.available = (g_sql.open && g_sql.close && g_sql.exec && g_sql.prepare_v2 &&
                       g_sql.step && g_sql.finalize && g_sql.bind_int && g_sql.bind_double &&
                       g_sql.bind_text && g_sql.column_int && g_sql.column_double && g_sql.column_text);
    return g_sql.available;
}

int sp_sqlite_is_available(void) {
    return sp_sqlite_initialize();
}

const char *sp_sqlite_backend_name(void) {
    sp_sqlite_initialize();
    return g_sql.backend_name ? g_sql.backend_name : "Unavailable";
}

int sp_sqlite3_open(const char *filename, sqlite3 **ppDb) { return g_sql.open(filename, ppDb); }
int sp_sqlite3_close(sqlite3 *db) { return g_sql.close(db); }
int sp_sqlite3_exec(sqlite3 *db, const char *sql, int (*cb)(void*,int,char**,char**), void *arg, char **errmsg) {
    return g_sql.exec(db, sql, cb, arg, errmsg);
}
void sp_sqlite3_free(void *ptr) { if (g_sql.free_fn && ptr) g_sql.free_fn(ptr); }
const char *sp_sqlite3_errmsg(sqlite3 *db) { return g_sql.errmsg ? g_sql.errmsg(db) : "SQLite error"; }
sqlite3_int64 sp_sqlite3_last_insert_rowid(sqlite3 *db) { return g_sql.last_rowid ? g_sql.last_rowid(db) : 0; }
int sp_sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail) {
    return g_sql.prepare_v2(db, zSql, nByte, ppStmt, pzTail);
}
int sp_sqlite3_step(sqlite3_stmt *pStmt) { return g_sql.step(pStmt); }
int sp_sqlite3_reset(sqlite3_stmt *pStmt) { return g_sql.reset ? g_sql.reset(pStmt) : 0; }
int sp_sqlite3_finalize(sqlite3_stmt *pStmt) { return g_sql.finalize(pStmt); }
int sp_sqlite3_bind_int(sqlite3_stmt *pStmt, int idx, int val) { return g_sql.bind_int(pStmt, idx, val); }
int sp_sqlite3_bind_int64(sqlite3_stmt *pStmt, int idx, sqlite3_int64 val) { return g_sql.bind_int64(pStmt, idx, val); }
int sp_sqlite3_bind_double(sqlite3_stmt *pStmt, int idx, double val) { return g_sql.bind_double(pStmt, idx, val); }
int sp_sqlite3_bind_text(sqlite3_stmt *pStmt, int idx, const char *val, int n, void(*d)(void*)) {
    return g_sql.bind_text(pStmt, idx, val, n, d);
}
int sp_sqlite3_column_int(sqlite3_stmt *pStmt, int iCol) { return g_sql.column_int(pStmt, iCol); }
sqlite3_int64 sp_sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol) { return g_sql.column_int64(pStmt, iCol); }
double sp_sqlite3_column_double(sqlite3_stmt *pStmt, int iCol) { return g_sql.column_double(pStmt, iCol); }
const unsigned char *sp_sqlite3_column_text(sqlite3_stmt *pStmt, int iCol) { return g_sql.column_text(pStmt, iCol); }
sqlite3_backup *sp_sqlite3_backup_init(sqlite3 *pDest, const char *zDest, sqlite3 *pSrc, const char *zSrc) {
    return g_sql.backup_init ? g_sql.backup_init(pDest, zDest, pSrc, zSrc) : NULL;
}
int sp_sqlite3_backup_step(sqlite3_backup *p, int nPage) { return g_sql.backup_step ? g_sql.backup_step(p, nPage) : -1; }
int sp_sqlite3_backup_finish(sqlite3_backup *p) { return g_sql.backup_finish ? g_sql.backup_finish(p) : -1; }

static void ensure_parent_dir(const char *file_path) {
    if (!file_path) return;
    char dir[260];
    stock_copy_str(dir, sizeof(dir), file_path);
    char *slash = strrchr(dir, '/');
    char *bslash = strrchr(dir, '\\');
    char *sep = (slash > bslash) ? slash : bslash;
    if (sep) {
        *sep = '\0';
        if (dir[0]) {
#ifdef _WIN32
            _mkdir(dir);
#else
            mkdir(dir, 0755);
#endif
        }
    }
}

static const char *DDL_SCHEMA =
    "PRAGMA foreign_keys = ON;"
    "CREATE TABLE IF NOT EXISTS users ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  username TEXT NOT NULL UNIQUE,"
    "  email TEXT NOT NULL,"
    "  currency TEXT NOT NULL DEFAULT 'INR',"
    "  cash_balance REAL NOT NULL DEFAULT 1000000.0,"
    "  created_at TEXT NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS stocks ("
    "  id INTEGER PRIMARY KEY,"
    "  ticker TEXT NOT NULL UNIQUE,"
    "  name TEXT NOT NULL,"
    "  sector TEXT NOT NULL DEFAULT 'General',"
    "  current_price REAL NOT NULL CHECK(current_price > 0.0),"
    "  previous_price REAL NOT NULL DEFAULT 0.0"
    ");"
    "CREATE TABLE IF NOT EXISTS holdings ("
    "  user_id INTEGER NOT NULL,"
    "  stock_id INTEGER NOT NULL,"
    "  quantity INTEGER NOT NULL DEFAULT 0 CHECK(quantity >= 0),"
    "  average_cost REAL NOT NULL DEFAULT 0.0 CHECK(average_cost >= 0.0),"
    "  realized_pl REAL NOT NULL DEFAULT 0.0,"
    "  PRIMARY KEY (user_id, stock_id),"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS transactions ("
    "  id INTEGER PRIMARY KEY,"
    "  user_id INTEGER NOT NULL,"
    "  stock_id INTEGER NOT NULL,"
    "  ticker TEXT NOT NULL,"
    "  type TEXT NOT NULL CHECK(type IN ('BUY','SELL')),"
    "  quantity INTEGER NOT NULL CHECK(quantity > 0),"
    "  price REAL NOT NULL CHECK(price > 0.0),"
    "  total_amount REAL NOT NULL CHECK(total_amount > 0.0),"
    "  realized_pl REAL NOT NULL DEFAULT 0.0,"
    "  timestamp TEXT NOT NULL,"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS watchlist ("
    "  user_id INTEGER NOT NULL,"
    "  stock_id INTEGER NOT NULL,"
    "  category TEXT NOT NULL DEFAULT 'General',"
    "  PRIMARY KEY (user_id, stock_id),"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS price_alerts ("
    "  id INTEGER PRIMARY KEY,"
    "  user_id INTEGER NOT NULL,"
    "  stock_id INTEGER NOT NULL,"
    "  ticker TEXT NOT NULL,"
    "  target_price REAL NOT NULL CHECK(target_price > 0.0),"
    "  condition_type INTEGER NOT NULL,"
    "  triggered INTEGER NOT NULL DEFAULT 0,"
    "  notes TEXT NOT NULL DEFAULT '',"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (stock_id) REFERENCES stocks(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS portfolio_snapshots ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  user_id INTEGER NOT NULL,"
    "  timestamp TEXT NOT NULL,"
    "  total_invested REAL NOT NULL,"
    "  total_market_value REAL NOT NULL,"
    "  unrealized_pl REAL NOT NULL,"
    "  realized_pl REAL NOT NULL,"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
    ");";

static int exec_sql(sqlite3 *db, const char *sql) {
    char *errmsg = NULL;
    int rc = sp_sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        set_db_error(errmsg ? errmsg : sp_sqlite3_errmsg(db));
        sp_sqlite3_free(errmsg);
        return 0;
    }
    return 1;
}

int db_init(const char *db_path) {
    if (!db_path || !*db_path) return 0;
    ensure_parent_dir(db_path);
    if (!sp_sqlite_initialize()) {
        set_db_error("SQLite3 library not available");
        return 0;
    }
    sqlite3 *db = NULL;
    if (sp_sqlite3_open(db_path, &db) != SQLITE_OK) {
        set_db_error(db ? sp_sqlite3_errmsg(db) : "Cannot open SQLite DB");
        if (db) sp_sqlite3_close(db);
        return 0;
    }
    int ok = exec_sql(db, DDL_SCHEMA);
    sp_sqlite3_close(db);
    return ok;
}

int db_save_portfolio(const Portfolio *p, const char *db_path) {
    if (!p || !db_path || !*db_path) return 0;
    if (!db_init(db_path)) return 0;

    sqlite3 *db = NULL;
    if (sp_sqlite3_open(db_path, &db) != SQLITE_OK) {
        set_db_error("Failed to open database for saving");
        return 0;
    }
    if (!exec_sql(db, "PRAGMA foreign_keys = ON; BEGIN IMMEDIATE TRANSACTION;")) {
        sp_sqlite3_close(db);
        return 0;
    }

    /* Clear dependent tables and re-insert within atomic ACID transaction */
    const char *clear_sql =
        "DELETE FROM portfolio_snapshots;"
        "DELETE FROM price_alerts;"
        "DELETE FROM watchlist;"
        "DELETE FROM transactions;"
        "DELETE FROM holdings;"
        "DELETE FROM stocks;"
        "DELETE FROM users;";
    if (!exec_sql(db, clear_sql)) {
        exec_sql(db, "ROLLBACK;");
        sp_sqlite3_close(db);
        return 0;
    }

    /* 1. Insert User Profile */
    sqlite3_stmt *stmt = NULL;
    const char *user_sql = "INSERT INTO users (id, username, email, currency, cash_balance, created_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    if (sp_sqlite3_prepare_v2(db, user_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
    sp_sqlite3_bind_int(stmt, 1, p->user.id > 0 ? p->user.id : 1);
    sp_sqlite3_bind_text(stmt, 2, p->user.username, -1, SQLITE_STATIC);
    sp_sqlite3_bind_text(stmt, 3, p->user.email, -1, SQLITE_STATIC);
    sp_sqlite3_bind_text(stmt, 4, p->user.currency, -1, SQLITE_STATIC);
    sp_sqlite3_bind_double(stmt, 5, p->user.cash_balance);
    sp_sqlite3_bind_text(stmt, 6, p->user.created_at, -1, SQLITE_STATIC);
    if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
    sp_sqlite3_finalize(stmt);

    /* 2. Insert Stocks, Holdings, and Watchlist */
    const char *stock_sql = "INSERT INTO stocks (id, ticker, name, sector, current_price, previous_price) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    const char *hold_sql  = "INSERT INTO holdings (user_id, stock_id, quantity, average_cost, realized_pl) VALUES (?1, ?2, ?3, ?4, ?5);";
    const char *wl_sql    = "INSERT INTO watchlist (user_id, stock_id, category) VALUES (?1, ?2, ?3);";

    for (size_t i = 0; i < p->stocks.count; ++i) {
        const Stock *s = &p->stocks.items[i];
        int sid = s->id > 0 ? s->id : (int)(i + 1);

        if (sp_sqlite3_prepare_v2(db, stock_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
        sp_sqlite3_bind_int(stmt, 1, sid);
        sp_sqlite3_bind_text(stmt, 2, s->ticker, -1, SQLITE_STATIC);
        sp_sqlite3_bind_text(stmt, 3, s->name, -1, SQLITE_STATIC);
        sp_sqlite3_bind_text(stmt, 4, s->sector, -1, SQLITE_STATIC);
        sp_sqlite3_bind_double(stmt, 5, s->current_price);
        sp_sqlite3_bind_double(stmt, 6, s->previous_price > 0.0 ? s->previous_price : s->current_price);
        if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
        sp_sqlite3_finalize(stmt);

        if (sp_sqlite3_prepare_v2(db, hold_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
        sp_sqlite3_bind_int(stmt, 1, p->user.id > 0 ? p->user.id : 1);
        sp_sqlite3_bind_int(stmt, 2, sid);
        sp_sqlite3_bind_int(stmt, 3, s->quantity);
        sp_sqlite3_bind_double(stmt, 4, s->average_cost);
        sp_sqlite3_bind_double(stmt, 5, s->realized_pl);
        if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
        sp_sqlite3_finalize(stmt);

        if (s->in_watchlist) {
            if (sp_sqlite3_prepare_v2(db, wl_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
            sp_sqlite3_bind_int(stmt, 1, p->user.id > 0 ? p->user.id : 1);
            sp_sqlite3_bind_int(stmt, 2, sid);
            sp_sqlite3_bind_text(stmt, 3, s->sector, -1, SQLITE_STATIC);
            if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
            sp_sqlite3_finalize(stmt);
        }
    }

    /* 3. Insert Transactions */
    const char *tx_sql = "INSERT INTO transactions (id, user_id, stock_id, ticker, type, quantity, price, total_amount, realized_pl, timestamp) "
                         "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);";
    for (const TransactionNode *tx = p->transactions.head; tx != NULL; tx = tx->next) {
        size_t sidx = 0;
        int sid = tx->stock_id;
        if (portfolio_find_stock(p, tx->ticker, &sidx)) {
            sid = p->stocks.items[sidx].id;
        }
        if (sid <= 0) continue;

        if (sp_sqlite3_prepare_v2(db, tx_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
        sp_sqlite3_bind_int64(stmt, 1, (sqlite3_int64)tx->id);
        sp_sqlite3_bind_int(stmt, 2, p->user.id > 0 ? p->user.id : 1);
        sp_sqlite3_bind_int(stmt, 3, sid);
        sp_sqlite3_bind_text(stmt, 4, tx->ticker, -1, SQLITE_STATIC);
        sp_sqlite3_bind_text(stmt, 5, tx->type, -1, SQLITE_STATIC);
        sp_sqlite3_bind_int(stmt, 6, tx->quantity);
        sp_sqlite3_bind_double(stmt, 7, tx->price);
        sp_sqlite3_bind_double(stmt, 8, tx->total);
        sp_sqlite3_bind_double(stmt, 9, tx->realized_pl);
        sp_sqlite3_bind_text(stmt, 10, tx->timestamp, -1, SQLITE_STATIC);
        if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
        sp_sqlite3_finalize(stmt);
    }

    /* 4. Insert Price Alerts */
    const char *alert_sql = "INSERT INTO price_alerts (id, user_id, stock_id, ticker, target_price, condition_type, triggered, notes) "
                            "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";
    for (size_t i = 0; i < p->price_alerts.size; ++i) {
        const PriceAlert *a = &p->price_alerts.items[i];
        size_t sidx = 0;
        if (!portfolio_find_stock(p, a->ticker, &sidx)) continue;
        int sid = p->stocks.items[sidx].id;

        if (sp_sqlite3_prepare_v2(db, alert_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
        sp_sqlite3_bind_int(stmt, 1, a->id);
        sp_sqlite3_bind_int(stmt, 2, p->user.id > 0 ? p->user.id : 1);
        sp_sqlite3_bind_int(stmt, 3, sid);
        sp_sqlite3_bind_text(stmt, 4, a->ticker, -1, SQLITE_STATIC);
        sp_sqlite3_bind_double(stmt, 5, a->target_price);
        sp_sqlite3_bind_int(stmt, 6, (int)a->condition);
        sp_sqlite3_bind_int(stmt, 7, a->triggered);
        sp_sqlite3_bind_text(stmt, 8, a->notes, -1, SQLITE_STATIC);
        if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
        sp_sqlite3_finalize(stmt);
    }

    /* 5. Insert Portfolio Snapshots */
    const char *snap_sql = "INSERT INTO portfolio_snapshots (user_id, timestamp, total_invested, total_market_value, unrealized_pl, realized_pl) "
                           "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    for (size_t i = 0; i < p->snapshot_count; ++i) {
        const PortfolioSnapshot *sn = &p->snapshots[i];
        if (sp_sqlite3_prepare_v2(db, snap_sql, -1, &stmt, NULL) != SQLITE_OK) goto rollback;
        sp_sqlite3_bind_int(stmt, 1, p->user.id > 0 ? p->user.id : 1);
        sp_sqlite3_bind_text(stmt, 2, sn->timestamp, -1, SQLITE_STATIC);
        sp_sqlite3_bind_double(stmt, 3, sn->total_invested);
        sp_sqlite3_bind_double(stmt, 4, sn->total_market_value);
        sp_sqlite3_bind_double(stmt, 5, sn->unrealized_pl);
        sp_sqlite3_bind_double(stmt, 6, sn->realized_pl);
        if (sp_sqlite3_step(stmt) != SQLITE_DONE) { sp_sqlite3_finalize(stmt); goto rollback; }
        sp_sqlite3_finalize(stmt);
    }

    if (!exec_sql(db, "COMMIT;")) goto rollback;
    sp_sqlite3_close(db);
    return 1;

rollback:
    set_db_error(sp_sqlite3_errmsg(db));
    exec_sql(db, "ROLLBACK;");
    sp_sqlite3_close(db);
    return 0;
}

int db_load_portfolio(Portfolio *p, const char *db_path) {
    if (!p || !db_path || !*db_path) return 0;
    if (!sp_sqlite_initialize()) return 0;

    FILE *probe = fopen(db_path, "rb");
    if (!probe) return 0;
    fclose(probe);

    sqlite3 *db = NULL;
    if (sp_sqlite3_open(db_path, &db) != SQLITE_OK) {
        if (db) sp_sqlite3_close(db);
        return 0;
    }

    sqlite3_stmt *stmt = NULL;

    /* Check if stocks table exists and has rows */
    if (sp_sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM stocks;", -1, &stmt, NULL) != SQLITE_OK) {
        sp_sqlite3_close(db);
        return 0;
    }
    int stock_rows = 0;
    if (sp_sqlite3_step(stmt) == SQLITE_ROW) {
        stock_rows = sp_sqlite3_column_int(stmt, 0);
    }
    sp_sqlite3_finalize(stmt);
    if (stock_rows <= 0) {
        sp_sqlite3_close(db);
        return 0;
    }

    /* Reset current portfolio data before loading */
    stock_array_destroy(&p->stocks);
    stock_array_init(&p->stocks, 32);
    hashmap_clear(&p->ticker_index);
    avl_clear(&p->avl_index);
    dll_clear(&p->transactions);
    stack_clear(&p->undo_stack);
    queue_clear(&p->pending_queue);
    heap_clear(&p->price_alerts);
    graph_clear(&p->correlation_graph);
    p->snapshot_count = 0;
    p->next_stock_id = 1;
    p->next_transaction_id = 1;
    stock_copy_str(p->db_path, sizeof(p->db_path), db_path);

    /* 1. Load User */
    if (sp_sqlite3_prepare_v2(db, "SELECT id, username, email, currency, cash_balance, created_at FROM users LIMIT 1;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            p->user.id = sp_sqlite3_column_int(stmt, 0);
            const char *u = (const char *)sp_sqlite3_column_text(stmt, 1);
            const char *e = (const char *)sp_sqlite3_column_text(stmt, 2);
            const char *c = (const char *)sp_sqlite3_column_text(stmt, 3);
            p->user.cash_balance = sp_sqlite3_column_double(stmt, 4);
            const char *ca = (const char *)sp_sqlite3_column_text(stmt, 5);
            if (u) stock_copy_str(p->user.username, sizeof(p->user.username), u);
            if (e) stock_copy_str(p->user.email, sizeof(p->user.email), e);
            if (c) stock_copy_str(p->user.currency, sizeof(p->user.currency), c);
            if (ca) stock_copy_str(p->user.created_at, sizeof(p->user.created_at), ca);
        }
        sp_sqlite3_finalize(stmt);
    }

    /* 2. Load Stocks + Holdings + Watchlist via LEFT JOIN */
    const char *load_stocks_sql =
        "SELECT s.id, s.ticker, s.name, s.sector, s.current_price, s.previous_price, "
        "       COALESCE(h.quantity, 0), COALESCE(h.average_cost, 0.0), COALESCE(h.realized_pl, 0.0), "
        "       CASE WHEN w.stock_id IS NOT NULL THEN 1 ELSE 0 END "
        "FROM stocks s "
        "LEFT JOIN holdings h ON h.stock_id = s.id AND h.user_id = ?1 "
        "LEFT JOIN watchlist w ON w.stock_id = s.id AND w.user_id = ?1 "
        "ORDER BY s.id ASC;";
    if (sp_sqlite3_prepare_v2(db, load_stocks_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sp_sqlite3_bind_int(stmt, 1, p->user.id > 0 ? p->user.id : 1);
        while (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            Stock item;
            memset(&item, 0, sizeof(item));
            item.id = sp_sqlite3_column_int(stmt, 0);
            const char *tk = (const char *)sp_sqlite3_column_text(stmt, 1);
            const char *nm = (const char *)sp_sqlite3_column_text(stmt, 2);
            const char *sc = (const char *)sp_sqlite3_column_text(stmt, 3);
            item.current_price = sp_sqlite3_column_double(stmt, 4);
            item.previous_price = sp_sqlite3_column_double(stmt, 5);
            item.quantity = sp_sqlite3_column_int(stmt, 6);
            item.average_cost = sp_sqlite3_column_double(stmt, 7);
            item.realized_pl = sp_sqlite3_column_double(stmt, 8);
            item.in_watchlist = sp_sqlite3_column_int(stmt, 9);
            if (tk) stock_copy_str(item.ticker, sizeof(item.ticker), tk);
            if (nm) stock_copy_str(item.name, sizeof(item.name), nm);
            if (sc) stock_copy_str(item.sector, sizeof(item.sector), sc);
            stock_recalculate_metrics(&item);
            stock_array_push(&p->stocks, &item, NULL);
            if (item.id >= p->next_stock_id) p->next_stock_id = item.id + 1;
        }
        sp_sqlite3_finalize(stmt);
    }
    portfolio_rebuild_indices(p);

    /* 3. Load Transactions */
    const char *load_tx_sql =
        "SELECT id, user_id, stock_id, ticker, type, quantity, price, total_amount, realized_pl, timestamp "
        "FROM transactions ORDER BY id ASC;";
    if (sp_sqlite3_prepare_v2(db, load_tx_sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            TransactionNode tx;
            memset(&tx, 0, sizeof(tx));
            tx.id = (unsigned long)sp_sqlite3_column_int64(stmt, 0);
            tx.user_id = sp_sqlite3_column_int(stmt, 1);
            tx.stock_id = sp_sqlite3_column_int(stmt, 2);
            const char *tk = (const char *)sp_sqlite3_column_text(stmt, 3);
            const char *tp = (const char *)sp_sqlite3_column_text(stmt, 4);
            tx.quantity = sp_sqlite3_column_int(stmt, 5);
            tx.price = sp_sqlite3_column_double(stmt, 6);
            tx.total = sp_sqlite3_column_double(stmt, 7);
            tx.realized_pl = sp_sqlite3_column_double(stmt, 8);
            const char *ts = (const char *)sp_sqlite3_column_text(stmt, 9);
            if (tk) stock_copy_str(tx.ticker, sizeof(tx.ticker), tk);
            if (tp) stock_copy_str(tx.type, sizeof(tx.type), tp);
            if (ts) stock_copy_str(tx.timestamp, sizeof(tx.timestamp), ts);
            dll_append(&p->transactions, &tx);
            if (tx.id >= p->next_transaction_id) p->next_transaction_id = tx.id + 1;
        }
        sp_sqlite3_finalize(stmt);
    }

    /* 4. Load Price Alerts into Min-Heap */
    const char *load_alert_sql =
        "SELECT id, ticker, target_price, condition_type, triggered, notes FROM price_alerts ORDER BY id ASC;";
    if (sp_sqlite3_prepare_v2(db, load_alert_sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            PriceAlert a;
            memset(&a, 0, sizeof(a));
            a.id = sp_sqlite3_column_int(stmt, 0);
            const char *tk = (const char *)sp_sqlite3_column_text(stmt, 1);
            a.target_price = sp_sqlite3_column_double(stmt, 2);
            a.condition = (AlertCondition)sp_sqlite3_column_int(stmt, 3);
            a.triggered = sp_sqlite3_column_int(stmt, 4);
            const char *nt = (const char *)sp_sqlite3_column_text(stmt, 5);
            if (tk) stock_copy_str(a.ticker, sizeof(a.ticker), tk);
            if (nt) stock_copy_str(a.notes, sizeof(a.notes), nt);
            size_t sidx = 0;
            if (portfolio_find_stock(p, a.ticker, &sidx)) {
                a.current_price = p->stocks.items[sidx].current_price;
            }
            heap_push(&p->price_alerts, &a);
        }
        sp_sqlite3_finalize(stmt);
    }

    /* 5. Load Portfolio Snapshots */
    const char *load_snap_sql =
        "SELECT id, timestamp, total_invested, total_market_value, unrealized_pl, realized_pl "
        "FROM portfolio_snapshots ORDER BY id ASC;";
    if (sp_sqlite3_prepare_v2(db, load_snap_sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sp_sqlite3_step(stmt) == SQLITE_ROW && p->snapshot_count < MAX_SNAPSHOTS) {
            PortfolioSnapshot *sn = &p->snapshots[p->snapshot_count++];
            sn->id = sp_sqlite3_column_int(stmt, 0);
            const char *ts = (const char *)sp_sqlite3_column_text(stmt, 1);
            sn->total_invested = sp_sqlite3_column_double(stmt, 2);
            sn->total_market_value = sp_sqlite3_column_double(stmt, 3);
            sn->unrealized_pl = sp_sqlite3_column_double(stmt, 4);
            sn->realized_pl = sp_sqlite3_column_double(stmt, 5);
            if (ts) stock_copy_str(sn->timestamp, sizeof(sn->timestamp), ts);
        }
        sp_sqlite3_finalize(stmt);
    }

    sp_sqlite3_close(db);
    return 1;
}

int db_record_trade_atomic(const Portfolio *p, const Stock *updated_stock,
                           const TransactionNode *tx, const char *db_path) {
    if (!p || !updated_stock || !tx || !db_path) return 0;
    return db_save_portfolio(p, db_path);
}

int db_backup(const char *source_db_path, const char *backup_db_path) {
    if (!source_db_path || !backup_db_path || !*source_db_path || !*backup_db_path) return 0;
    ensure_parent_dir(backup_db_path);
    if (!sp_sqlite_initialize()) return 0;

    sqlite3 *src_db = NULL;
    sqlite3 *dst_db = NULL;
    if (sp_sqlite3_open(source_db_path, &src_db) != SQLITE_OK) {
        if (src_db) sp_sqlite3_close(src_db);
        return 0;
    }
    if (sp_sqlite3_open(backup_db_path, &dst_db) != SQLITE_OK) {
        if (dst_db) sp_sqlite3_close(dst_db);
        sp_sqlite3_close(src_db);
        return 0;
    }

    int ok = 0;
    if (g_sql.backup_init && g_sql.backup_step && g_sql.backup_finish) {
        sqlite3_backup *bk = sp_sqlite3_backup_init(dst_db, "main", src_db, "main");
        if (bk) {
            int rc = sp_sqlite3_backup_step(bk, -1);
            sp_sqlite3_backup_finish(bk);
            if (rc == SQLITE_DONE) ok = 1;
        }
    }
    sp_sqlite3_close(dst_db);
    sp_sqlite3_close(src_db);
    return ok;
}

int db_restore(const char *backup_db_path, const char *target_db_path, Portfolio *p) {
    if (!backup_db_path || !target_db_path) return 0;
    if (!db_backup(backup_db_path, target_db_path)) return 0;
    if (p) {
        return db_load_portfolio(p, target_db_path);
    }
    return 1;
}

int db_integrity_check(const char *db_path, char *result_buf, size_t buf_size) {
    if (!db_path || !*db_path) return 0;
    if (!sp_sqlite_initialize()) return 0;

    sqlite3 *db = NULL;
    if (sp_sqlite3_open(db_path, &db) != SQLITE_OK) {
        if (db) sp_sqlite3_close(db);
        return 0;
    }

    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char pragma_res[128] = "unknown";
    if (sp_sqlite3_prepare_v2(db, "PRAGMA integrity_check;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            const char *txt = (const char *)sp_sqlite3_column_text(stmt, 0);
            if (txt) {
                stock_copy_str(pragma_res, sizeof(pragma_res), txt);
                if (stock_stricmp(txt, "ok") == 0) ok = 1;
            }
        }
        sp_sqlite3_finalize(stmt);
    }

    int fk_violations = 0;
    if (sp_sqlite3_prepare_v2(db, "PRAGMA foreign_key_check;", -1, &stmt, NULL) == SQLITE_OK) {
        while (sp_sqlite3_step(stmt) == SQLITE_ROW) {
            fk_violations++;
        }
        sp_sqlite3_finalize(stmt);
    }
    if (fk_violations > 0) ok = 0;

    if (result_buf && buf_size > 0) {
        snprintf(result_buf, buf_size, "SQLite Engine (%s) PRAGMA integrity_check: %s | Foreign Key Violations: %d",
                 sp_sqlite_backend_name(), pragma_res, fk_violations);
    }
    sp_sqlite3_close(db);
    return ok;
}
