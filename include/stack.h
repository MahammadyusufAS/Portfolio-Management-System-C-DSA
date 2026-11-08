#ifndef STACK_H
#define STACK_H

#include <stddef.h>

typedef struct {
    unsigned long transaction_id;
    int stock_id;
    char ticker[24];
    char type[8]; /* "BUY" or "SELL" */
    int quantity_delta;
    double trade_price;
    int previous_quantity;
    double previous_average_cost;
    double previous_realized_pl;
    double realized_pl_delta;
} UndoRecord;

typedef struct {
    UndoRecord *items;
    size_t size;
    size_t capacity;
} UndoStack;

void stack_init(UndoStack *stack);
int stack_push(UndoStack *stack, const UndoRecord *record);
int stack_pop(UndoStack *stack, UndoRecord *record_out);
int stack_peek(const UndoStack *stack, UndoRecord *record_out);
int stack_is_empty(const UndoStack *stack);
void stack_clear(UndoStack *stack);

#endif /* STACK_H */
