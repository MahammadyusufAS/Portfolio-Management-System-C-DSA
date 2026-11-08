#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stddef.h>

typedef struct AVLNode {
    char ticker[24];
    size_t stock_index;
    double price;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    AVLNode *root;
    size_t size;
    size_t rotation_count;
} AVLTree;

typedef void (*AVLVisitFn)(const char *ticker, size_t stock_index, double price, void *user_data);

void avl_init(AVLTree *tree);
int avl_insert(AVLTree *tree, const char *ticker, size_t stock_index, double price);
int avl_remove(AVLTree *tree, const char *ticker);
int avl_search(const AVLTree *tree, const char *ticker, size_t *stock_index_out);
int avl_search_with_steps(const AVLTree *tree, const char *ticker, size_t *stock_index_out, size_t *steps_out);
size_t avl_range_query_ticker(const AVLTree *tree, const char *low_ticker, const char *high_ticker,
                              size_t *indices_out, size_t max_out);
size_t avl_range_query_price(const AVLTree *tree, double min_price, double max_price,
                             size_t *indices_out, size_t max_out);
void avl_inorder(const AVLTree *tree, AVLVisitFn visit, void *user_data);
int avl_height(const AVLTree *tree);
int avl_is_balanced(const AVLTree *tree);
void avl_clear(AVLTree *tree);

#endif /* AVL_TREE_H */
