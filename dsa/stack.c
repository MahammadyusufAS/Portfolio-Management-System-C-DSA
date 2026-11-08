#include "stack.h"
#include <stdlib.h>
#include <string.h>

/*
 * LIFO Stack for Undo Operations
 * Stores reversible transaction state snapshots in a dynamically resized buffer.
 */

void stack_init(UndoStack *stack) {
    if (!stack) return;
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

int stack_push(UndoStack *stack, const UndoRecord *record) {
    if (!stack || !record) return 0;
    if (stack->size >= stack->capacity) {
        size_t new_cap = stack->capacity ? stack->capacity * 2 : 16;
        UndoRecord *new_items = (UndoRecord *)realloc(stack->items, new_cap * sizeof(UndoRecord));
        if (!new_items) return 0;
        stack->items = new_items;
        stack->capacity = new_cap;
    }
    stack->items[stack->size++] = *record;
    return 1;
}

int stack_pop(UndoStack *stack, UndoRecord *record_out) {
    if (!stack || stack->size == 0) return 0;
    stack->size--;
    if (record_out) {
        *record_out = stack->items[stack->size];
    }
    return 1;
}

int stack_peek(const UndoStack *stack, UndoRecord *record_out) {
    if (!stack || stack->size == 0) return 0;
    if (record_out) {
        *record_out = stack->items[stack->size - 1];
    }
    return 1;
}

int stack_is_empty(const UndoStack *stack) {
    return (!stack || stack->size == 0);
}

void stack_clear(UndoStack *stack) {
    if (!stack) return;
    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}
