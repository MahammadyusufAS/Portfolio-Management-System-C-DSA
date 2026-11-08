#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "stock.h"
#include "hashmap.h"
#include "avl_tree.h"
#include "linkedlist.h"
#include "stack.h"
#include "queue.h"
#include "heap.h"
#include "sorting.h"
#include "searching.h"
#include "graph.h"
#include "transaction.h"
#include <stddef.h>

#define MAX_SNAPSHOTS 128
#define MAX_SECTORS   32

typedef struct {
    int id;
    char username[48];
    char email[80];
    char currency[8];
    double cash_balance;
    char created_at[32];
} UserProfile;

typedef struct {
    int id;
    char timestamp[32];
    double total_invested;
    double total_market_value;
    double unrealized_pl;
    double realized_pl;
} PortfolioSnapshot;

typedef struct {
    char sector[40];
    double invested_value;
    double market_value;
    double weight_percent;
    double unrealized_pl;
    size_t stock_count;
} SectorAllocation;

typedef struct {
    double total_market_value;
    double total_invested;
    double total_unrealized_pl;
    double total_realized_pl;
    double net_total_pl;
    double overall_return_pct;
    double diversification_score; /* 0..100 based on HHI + Graph correlation */
    double avg_pair_correlation;
    size_t active_holdings_count;
    size_t catalog_stocks_count;
    size_t watchlist_count;
    int best_stock_index;
    int worst_stock_index;
    SectorAllocation sectors[MAX_SECTORS];
    size_t sector_count;
} PortfolioAnalytics;

typedef struct {
    UserProfile user;
    StockArray stocks;            /* 1. Dynamic Array of stocks & holdings */
    HashMap ticker_index;         /* 3. Hash Table for O(1) ticker lookup */
    AVLTree avl_index;            /* 4. AVL Tree for O(log n) ordered lookup & range queries */
    TransactionList transactions; /* 2. Doubly Linked List for transaction history */
    UndoStack undo_stack;         /* 5. Stack for reversible trade operations */
    TaskQueue pending_queue;      /* 6. Queue for pending/scheduled operations */
    AlertMinHeap price_alerts;    /* 7. Min-Heap Priority Queue for price alerts */
    StockGraph correlation_graph; /* 10. Adjacency List Graph for stock correlations */
    PortfolioSnapshot snapshots[MAX_SNAPSHOTS];
    size_t snapshot_count;
    unsigned long next_transaction_id;
    int next_stock_id;
    char db_path[260];
} Portfolio;

void portfolio_init(Portfolio *p);
void portfolio_destroy(Portfolio *p);
void portfolio_rebuild_indices(Portfolio *p);

/* Stock Catalog & Watchlist Management */
int portfolio_find_stock(const Portfolio *p, const char *ticker, size_t *index_out);
int portfolio_add_stock(Portfolio *p, const char *ticker, const char *name, const char *sector, double price);
int portfolio_remove_stock(Portfolio *p, const char *ticker);
int portfolio_update_price(Portfolio *p, const char *ticker, double new_price);
int portfolio_set_watchlist(Portfolio *p, const char *ticker, int in_watchlist);

/* Buy / Sell / Undo / Queue Operations */
int portfolio_trade(Portfolio *p, const char *ticker, int quantity, double price, int is_buy);
int portfolio_undo(Portfolio *p);
int portfolio_enqueue_task(Portfolio *p, TaskType type, const char *ticker, int quantity, double price);
size_t portfolio_process_pending_queue(Portfolio *p, size_t max_tasks);

/* Price Alerts (Min-Heap) */
int portfolio_add_alert(Portfolio *p, const char *ticker, double target_price, AlertCondition cond, const char *notes);
int portfolio_remove_alert(Portfolio *p, int alert_id);

/* Snapshots & Analytics */
int portfolio_record_snapshot(Portfolio *p, const char *custom_timestamp);
double portfolio_market_value(const Portfolio *p);
double portfolio_invested_value(const Portfolio *p);
double portfolio_unrealized_pl(const Portfolio *p);
double portfolio_realized_pl(const Portfolio *p);
size_t portfolio_active_holdings_count(const Portfolio *p);
void portfolio_compute_analytics(const Portfolio *p, PortfolioAnalytics *out);

/* Data Integrity Verification */
int portfolio_verify_integrity(const Portfolio *p, char *report_buf, size_t buf_size);

/* Seed Default Sample Data (clearly labeled sample prices) */
void portfolio_seed_sample_data(Portfolio *p);

#endif /* PORTFOLIO_H */
