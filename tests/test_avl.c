#include "avl_tree.h"
#include "stock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char prev_ticker[24];
    size_t visited_count;
    int sorted_ok;
} InorderCheck;

static void check_inorder_cb(const char *ticker, size_t stock_index, double price, void *user_data) {
    (void)stock_index; (void)price;
    InorderCheck *chk = (InorderCheck *)user_data;
    if (chk->visited_count > 0) {
        if (stock_stricmp(chk->prev_ticker, ticker) >= 0) {
            chk->sorted_ok = 0;
        }
    }
    stock_copy_str(chk->prev_ticker, sizeof(chk->prev_ticker), ticker);
    chk->visited_count++;
}

int main(void) {
    printf("=== Running Unit Tests: Self-Balancing AVL Tree ===\n");

    AVLTree tree;
    avl_init(&tree);

    /* 1. Test RR Rotation (Ascending Insertions: A, B, C) */
    assert(avl_insert(&tree, "AAA", 0, 100.0) == 1);
    assert(avl_insert(&tree, "BBB", 1, 200.0) == 1);
    assert(avl_insert(&tree, "CCC", 2, 300.0) == 1);
    assert(avl_is_balanced(&tree) == 1);
    assert(avl_height(&tree) == 2);
    assert(tree.rotation_count >= 1);
    avl_clear(&tree);

    /* 2. Test LL Rotation (Descending Insertions: C, B, A) */
    assert(avl_insert(&tree, "CCC", 2, 300.0) == 1);
    assert(avl_insert(&tree, "BBB", 1, 200.0) == 1);
    assert(avl_insert(&tree, "AAA", 0, 100.0) == 1);
    assert(avl_is_balanced(&tree) == 1);
    assert(avl_height(&tree) == 2);
    avl_clear(&tree);

    /* 3. Test LR Rotation (C, A, B) */
    assert(avl_insert(&tree, "CCC", 2, 300.0) == 1);
    assert(avl_insert(&tree, "AAA", 0, 100.0) == 1);
    assert(avl_insert(&tree, "BBB", 1, 200.0) == 1);
    assert(avl_is_balanced(&tree) == 1);
    assert(avl_height(&tree) == 2);
    avl_clear(&tree);

    /* 4. Test RL Rotation (A, C, B) */
    assert(avl_insert(&tree, "AAA", 0, 100.0) == 1);
    assert(avl_insert(&tree, "CCC", 2, 300.0) == 1);
    assert(avl_insert(&tree, "BBB", 1, 200.0) == 1);
    assert(avl_is_balanced(&tree) == 1);
    assert(avl_height(&tree) == 2);
    avl_clear(&tree);

    /* 5. Stress Test: 250 Sequential Insertions + In-Order Verification + 125 Deletions */
    char tk[24];
    for (size_t i = 0; i < 250; ++i) {
        snprintf(tk, sizeof(tk), "SYM%03zu", i);
        assert(avl_insert(&tree, tk, i, (double)(i + 1) * 10.0) == 1);
        assert(avl_is_balanced(&tree) == 1);
    }
    assert(tree.size == 250);
    assert(avl_height(&tree) <= 10); /* 1.44 * log2(250) ~= 11.4 */

    InorderCheck chk = { "", 0, 1 };
    avl_inorder(&tree, check_inorder_cb, &chk);
    assert(chk.visited_count == 250);
    assert(chk.sorted_ok == 1);

    /* Range query tests */
    size_t range_out[64];
    size_t rcount = avl_range_query_ticker(&tree, "SYM010", "SYM019", range_out, 64);
    assert(rcount == 10);

    size_t pcount = avl_range_query_price(&tree, 100.0, 200.0, range_out, 64);
    assert(pcount == 11); /* 100, 110, ..., 200 */

    /* Remove 125 nodes and verify balance after every single deletion */
    for (size_t i = 0; i < 250; i += 2) {
        snprintf(tk, sizeof(tk), "SYM%03zu", i);
        assert(avl_remove(&tree, tk) == 1);
        assert(avl_is_balanced(&tree) == 1);
    }
    assert(tree.size == 125);

    for (size_t i = 1; i < 250; i += 2) {
        snprintf(tk, sizeof(tk), "SYM%03zu", i);
        size_t idx = 0;
        assert(avl_search(&tree, tk, &idx) == 1 && idx == i);
    }

    avl_clear(&tree);
    assert(tree.size == 0);
    printf("[PASS] All AVL Tree rotation, balancing, and range query tests passed!\n");
    return 0;
}
