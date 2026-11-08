#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "linkedlist.h"
#include "stack.h"
#include <stddef.h>

typedef struct {
    unsigned long total_transactions;
    unsigned long buy_count;
    unsigned long sell_count;
    double total_buy_volume;
    double total_sell_volume;
    double net_realized_pl;
} TransactionSummary;

void transaction_make_timestamp(char *buffer, size_t size);
TransactionNode *transaction_record(TransactionList *list, unsigned long id, int user_id, int stock_id,
                                    const char *ticker, int is_buy, int quantity,
                                    double price, double realized_pl, const char *custom_timestamp);
int transaction_remove_last_by_id(TransactionList *list, unsigned long id);
void transaction_compute_summary(const TransactionList *list, TransactionSummary *summary_out);

#endif /* TRANSACTION_H */
