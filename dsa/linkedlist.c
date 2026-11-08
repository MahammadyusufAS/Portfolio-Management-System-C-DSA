#include "linkedlist.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>

/*
 * Doubly Linked List for Transaction History
 * Supports O(1) insertion at head/tail, O(1) node splicing, and bidirectional
 * traversal (forward chronological or reverse chronological) with multi-field filtering.
 */

void dll_init(TransactionList *list) {
    if (!list) return;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

TransactionNode *dll_append(TransactionList *list, const TransactionNode *item) {
    if (!list || !item) return NULL;
    TransactionNode *node = (TransactionNode *)calloc(1, sizeof(TransactionNode));
    if (!node) return NULL;
    *node = *item;
    node->prev = list->tail;
    node->next = NULL;
    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
    return node;
}

TransactionNode *dll_prepend(TransactionList *list, const TransactionNode *item) {
    if (!list || !item) return NULL;
    TransactionNode *node = (TransactionNode *)calloc(1, sizeof(TransactionNode));
    if (!node) return NULL;
    *node = *item;
    node->prev = NULL;
    node->next = list->head;
    if (list->head) {
        list->head->prev = node;
    } else {
        list->tail = node;
    }
    list->head = node;
    list->size++;
    return node;
}

TransactionNode *dll_find_by_id(const TransactionList *list, unsigned long id) {
    if (!list) return NULL;
    for (TransactionNode *curr = list->tail; curr != NULL; curr = curr->prev) {
        if (curr->id == id) return curr;
    }
    return NULL;
}

int dll_remove_node(TransactionList *list, TransactionNode *node) {
    if (!list || !node) return 0;
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }
    free(node);
    if (list->size > 0) list->size--;
    return 1;
}

int dll_remove_by_id(TransactionList *list, unsigned long id) {
    TransactionNode *node = dll_find_by_id(list, id);
    if (!node) return 0;
    return dll_remove_node(list, node);
}

size_t dll_filter(const TransactionList *list, const char *ticker_filter,
                  const char *type_filter, const char *start_date, const char *end_date,
                  const TransactionNode **out_nodes, size_t max_out, int reverse_order) {
    if (!list || !out_nodes || max_out == 0) return 0;
    size_t count = 0;
    const TransactionNode *curr = reverse_order ? list->tail : list->head;
    while (curr && count < max_out) {
        int match = 1;
        if (ticker_filter && *ticker_filter) {
            if (!stock_stristr(curr->ticker, ticker_filter)) match = 0;
        }
        if (match && type_filter && *type_filter && stock_stricmp(type_filter, "ALL") != 0) {
            if (stock_stricmp(curr->type, type_filter) != 0) match = 0;
        }
        if (match && start_date && *start_date) {
            /* Compare YYYY-MM-DD prefix (first 10 chars) */
            if (strncmp(curr->timestamp, start_date, strlen(start_date)) < 0) match = 0;
        }
        if (match && end_date && *end_date) {
            size_t elen = strlen(end_date);
            if (strncmp(curr->timestamp, end_date, elen) > 0) match = 0;
        }
        if (match) {
            out_nodes[count++] = curr;
        }
        curr = reverse_order ? curr->prev : curr->next;
    }
    return count;
}

void dll_clear(TransactionList *list) {
    if (!list) return;
    TransactionNode *curr = list->head;
    while (curr) {
        TransactionNode *next = curr->next;
        free(curr);
        curr = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}
