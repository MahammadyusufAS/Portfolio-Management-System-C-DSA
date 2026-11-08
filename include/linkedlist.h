#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stddef.h>

typedef struct TransactionNode {
    unsigned long id;
    int user_id;
    int stock_id;
    char ticker[24];
    char type[8]; /* "BUY" or "SELL" */
    int quantity;
    double price;
    double total;
    double realized_pl;
    char timestamp[32];
    struct TransactionNode *prev;
    struct TransactionNode *next;
} TransactionNode;

typedef struct {
    TransactionNode *head;
    TransactionNode *tail;
    size_t size;
} TransactionList;

void dll_init(TransactionList *list);
TransactionNode *dll_append(TransactionList *list, const TransactionNode *item);
TransactionNode *dll_prepend(TransactionList *list, const TransactionNode *item);
TransactionNode *dll_find_by_id(const TransactionList *list, unsigned long id);
int dll_remove_node(TransactionList *list, TransactionNode *node);
int dll_remove_by_id(TransactionList *list, unsigned long id);
size_t dll_filter(const TransactionList *list, const char *ticker_filter,
                  const char *type_filter, const char *start_date, const char *end_date,
                  const TransactionNode **out_nodes, size_t max_out, int reverse_order);
void dll_clear(TransactionList *list);

#endif /* LINKEDLIST_H */
