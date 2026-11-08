#include "avl_tree.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>

/*
 * Self-Balancing AVL Tree Implementation
 * Maintains stocks ordered alphabetically by ticker symbol with O(log n)
 * search, insertion, and deletion, plus ticker-range and price-range queries.
 */

static int node_height(const AVLNode *n) {
    return n ? n->height : 0;
}

static int max_int(int a, int b) {
    return (a > b) ? a : b;
}

static void update_height(AVLNode *n) {
    if (n) {
        n->height = 1 + max_int(node_height(n->left), node_height(n->right));
    }
}

static int balance_factor(const AVLNode *n) {
    return n ? (node_height(n->left) - node_height(n->right)) : 0;
}

static AVLNode *rotate_right(AVLTree *tree, AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    update_height(y);
    update_height(x);
    if (tree) tree->rotation_count++;
    return x;
}

static AVLNode *rotate_left(AVLTree *tree, AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    update_height(x);
    update_height(y);
    if (tree) tree->rotation_count++;
    return y;
}

static AVLNode *rebalance(AVLTree *tree, AVLNode *node) {
    if (!node) return NULL;
    update_height(node);
    int bf = balance_factor(node);

    /* Left Heavy */
    if (bf > 1) {
        if (balance_factor(node->left) < 0) {
            node->left = rotate_left(tree, node->left); /* LR Case */
        }
        return rotate_right(tree, node); /* LL Case */
    }
    /* Right Heavy */
    if (bf < -1) {
        if (balance_factor(node->right) > 0) {
            node->right = rotate_right(tree, node->right); /* RL Case */
        }
        return rotate_left(tree, node); /* RR Case */
    }
    return node;
}

static AVLNode *create_node(const char *ticker, size_t stock_index, double price) {
    AVLNode *node = (AVLNode *)calloc(1, sizeof(AVLNode));
    if (!node) return NULL;
    stock_normalize_ticker(ticker, node->ticker, sizeof(node->ticker));
    node->stock_index = stock_index;
    node->price = price;
    node->height = 1;
    return node;
}

static AVLNode *insert_rec(AVLTree *tree, AVLNode *node, const char *ticker,
                           size_t stock_index, double price, int *inserted_new, int *alloc_ok) {
    if (!node) {
        AVLNode *n = create_node(ticker, stock_index, price);
        if (!n) {
            *alloc_ok = 0;
            return NULL;
        }
        *inserted_new = 1;
        return n;
    }

    int cmp = stock_stricmp(ticker, node->ticker);
    if (cmp < 0) {
        node->left = insert_rec(tree, node->left, ticker, stock_index, price, inserted_new, alloc_ok);
    } else if (cmp > 0) {
        node->right = insert_rec(tree, node->right, ticker, stock_index, price, inserted_new, alloc_ok);
    } else {
        /* Update existing key */
        node->stock_index = stock_index;
        node->price = price;
        return node;
    }

    return rebalance(tree, node);
}

static AVLNode *min_value_node(AVLNode *node) {
    AVLNode *curr = node;
    while (curr && curr->left) curr = curr->left;
    return curr;
}

static AVLNode *remove_rec(AVLTree *tree, AVLNode *root, const char *ticker, int *removed) {
    if (!root) return NULL;

    int cmp = stock_stricmp(ticker, root->ticker);
    if (cmp < 0) {
        root->left = remove_rec(tree, root->left, ticker, removed);
    } else if (cmp > 0) {
        root->right = remove_rec(tree, root->right, ticker, removed);
    } else {
        *removed = 1;
        if (!root->left || !root->right) {
            AVLNode *temp = root->left ? root->left : root->right;
            if (!temp) {
                free(root);
                return NULL;
            } else {
                *root = *temp;
                free(temp);
            }
        } else {
            AVLNode *succ = min_value_node(root->right);
            stock_copy_str(root->ticker, sizeof(root->ticker), succ->ticker);
            root->stock_index = succ->stock_index;
            root->price = succ->price;
            int dummy = 0;
            root->right = remove_rec(tree, root->right, succ->ticker, &dummy);
        }
    }
    return rebalance(tree, root);
}

void avl_init(AVLTree *tree) {
    if (!tree) return;
    tree->root = NULL;
    tree->size = 0;
    tree->rotation_count = 0;
}

int avl_insert(AVLTree *tree, const char *ticker, size_t stock_index, double price) {
    if (!tree || !ticker || !*ticker) return 0;
    int inserted_new = 0;
    int alloc_ok = 1;
    tree->root = insert_rec(tree, tree->root, ticker, stock_index, price, &inserted_new, &alloc_ok);
    if (!alloc_ok) return 0;
    if (inserted_new) tree->size++;
    return 1;
}

int avl_remove(AVLTree *tree, const char *ticker) {
    if (!tree || !ticker || !*ticker) return 0;
    int removed = 0;
    tree->root = remove_rec(tree, tree->root, ticker, &removed);
    if (removed && tree->size > 0) tree->size--;
    return removed;
}

int avl_search_with_steps(const AVLTree *tree, const char *ticker, size_t *stock_index_out, size_t *steps_out) {
    size_t steps = 0;
    if (!tree || !ticker || !*ticker) {
        if (steps_out) *steps_out = 0;
        return 0;
    }
    const AVLNode *curr = tree->root;
    while (curr) {
        steps++;
        int cmp = stock_stricmp(ticker, curr->ticker);
        if (cmp == 0) {
            if (stock_index_out) *stock_index_out = curr->stock_index;
            if (steps_out) *steps_out = steps;
            return 1;
        }
        curr = (cmp < 0) ? curr->left : curr->right;
    }
    if (steps_out) *steps_out = steps;
    return 0;
}

int avl_search(const AVLTree *tree, const char *ticker, size_t *stock_index_out) {
    return avl_search_with_steps(tree, ticker, stock_index_out, NULL);
}

static void range_ticker_rec(const AVLNode *node, const char *low, const char *high,
                             size_t *out, size_t max_out, size_t *count) {
    if (!node || *count >= max_out) return;
    int cmp_low = stock_stricmp(node->ticker, low);
    int cmp_high = stock_stricmp(node->ticker, high);

    if (cmp_low > 0) {
        range_ticker_rec(node->left, low, high, out, max_out, count);
    }
    if (cmp_low >= 0 && cmp_high <= 0 && *count < max_out) {
        out[(*count)++] = node->stock_index;
    }
    if (cmp_high < 0) {
        range_ticker_rec(node->right, low, high, out, max_out, count);
    }
}

size_t avl_range_query_ticker(const AVLTree *tree, const char *low_ticker, const char *high_ticker,
                              size_t *indices_out, size_t max_out) {
    if (!tree || !low_ticker || !high_ticker || !indices_out || max_out == 0) return 0;
    size_t count = 0;
    range_ticker_rec(tree->root, low_ticker, high_ticker, indices_out, max_out, &count);
    return count;
}

static void range_price_rec(const AVLNode *node, double min_p, double max_p,
                            size_t *out, size_t max_out, size_t *count) {
    if (!node || *count >= max_out) return;
    range_price_rec(node->left, min_p, max_p, out, max_out, count);
    if (node->price >= min_p && node->price <= max_p && *count < max_out) {
        out[(*count)++] = node->stock_index;
    }
    range_price_rec(node->right, min_p, max_p, out, max_out, count);
}

size_t avl_range_query_price(const AVLTree *tree, double min_price, double max_price,
                             size_t *indices_out, size_t max_out) {
    if (!tree || !indices_out || max_out == 0 || min_price > max_price) return 0;
    size_t count = 0;
    range_price_rec(tree->root, min_price, max_price, indices_out, max_out, &count);
    return count;
}

static void inorder_rec(const AVLNode *node, AVLVisitFn visit, void *user_data) {
    if (!node) return;
    inorder_rec(node->left, visit, user_data);
    visit(node->ticker, node->stock_index, node->price, user_data);
    inorder_rec(node->right, visit, user_data);
}

void avl_inorder(const AVLTree *tree, AVLVisitFn visit, void *user_data) {
    if (!tree || !visit) return;
    inorder_rec(tree->root, visit, user_data);
}

int avl_height(const AVLTree *tree) {
    return tree ? node_height(tree->root) : 0;
}

static int check_balanced_rec(const AVLNode *node) {
    if (!node) return 1;
    int bf = balance_factor(node);
    if (bf < -1 || bf > 1) return 0;
    int expected_h = 1 + max_int(node_height(node->left), node_height(node->right));
    if (node->height != expected_h) return 0;
    return check_balanced_rec(node->left) && check_balanced_rec(node->right);
}

int avl_is_balanced(const AVLTree *tree) {
    if (!tree) return 1;
    return check_balanced_rec(tree->root);
}

static void clear_rec(AVLNode *node) {
    if (!node) return;
    clear_rec(node->left);
    clear_rec(node->right);
    free(node);
}

void avl_clear(AVLTree *tree) {
    if (!tree) return;
    clear_rec(tree->root);
    tree->root = NULL;
    tree->size = 0;
    tree->rotation_count = 0;
}
