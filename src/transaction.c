#include "transaction.h"
#include "stock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void transaction_make_timestamp(char *buffer, size_t size) {
    if (!buffer || size == 0) return;
    time_t now = time(NULL);
    const struct tm *tm_ptr = localtime(&now);
    if (tm_ptr) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_ptr);
    } else {
        stock_copy_str(buffer, size, "2026-11-08 09:30:00");
    }
}

TransactionNode *transaction_record(TransactionList *list, unsigned long id, int user_id, int stock_id,
                                    const char *ticker, int is_buy, int quantity,
                                    double price, double realized_pl, const char *custom_timestamp) {
    if (!list || !ticker || !*ticker || quantity <= 0 || price <= 0.0) return NULL;
    TransactionNode item;
    memset(&item, 0, sizeof(item));
    item.id = id;
    item.user_id = user_id > 0 ? user_id : 1;
    item.stock_id = stock_id;
    stock_normalize_ticker(ticker, item.ticker, sizeof(item.ticker));
    stock_copy_str(item.type, sizeof(item.type), is_buy ? "BUY" : "SELL");
    item.quantity = quantity;
    item.price = price;
    item.total = (double)quantity * price;
    item.realized_pl = realized_pl;
    if (custom_timestamp && *custom_timestamp) {
        stock_copy_str(item.timestamp, sizeof(item.timestamp), custom_timestamp);
    } else {
        transaction_make_timestamp(item.timestamp, sizeof(item.timestamp));
    }
    return dll_append(list, &item);
}

int transaction_remove_last_by_id(TransactionList *list, unsigned long id) {
    if (!list) return 0;
    return dll_remove_by_id(list, id);
}

void transaction_compute_summary(const TransactionList *list, TransactionSummary *summary_out) {
    if (!summary_out) return;
    memset(summary_out, 0, sizeof(*summary_out));
    if (!list) return;
    for (const TransactionNode *curr = list->head; curr != NULL; curr = curr->next) {
        summary_out->total_transactions++;
        if (stock_stricmp(curr->type, "BUY") == 0) {
            summary_out->buy_count++;
            summary_out->total_buy_volume += curr->total;
        } else {
            summary_out->sell_count++;
            summary_out->total_sell_volume += curr->total;
            summary_out->net_realized_pl += curr->realized_pl;
        }
    }
}
