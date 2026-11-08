#include "hashmap.h"
#include "searching.h"
#include "portfolio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("=== Running Unit Tests: Hash Table & Searching Algorithms ===\n");

    HashMap map;
    hashmap_init(&map);
    assert(map.size == 0);
    assert(map.collision_count == 0);

    /* 1. Basic Insert, Case-Insensitive Lookup, and Update */
    assert(hashmap_put(&map, "RELIANCE", 10) == 1);
    assert(hashmap_put(&map, "TCS", 20) == 1);
    assert(hashmap_put(&map, "INFY", 30) == 1);
    assert(map.size == 3);

    size_t val = 0;
    assert(hashmap_get(&map, "reliance", &val) == 1 && val == 10);
    assert(hashmap_get(&map, "tcs", &val) == 1 && val == 20);
    assert(hashmap_get(&map, "INFY", &val) == 1 && val == 30);
    assert(hashmap_get(&map, "NONEXISTENT", &val) == 0);

    /* Duplicate key update should not increase size */
    assert(hashmap_put(&map, "RELIANCE", 99) == 1);
    assert(map.size == 3);
    assert(hashmap_get(&map, "RELIANCE", &val) == 1 && val == 99);

    /* 2. Deletion */
    assert(hashmap_remove(&map, "TCS") == 1);
    assert(map.size == 2);
    assert(hashmap_get(&map, "TCS", &val) == 0);
    assert(hashmap_remove(&map, "TCS") == 0);

    hashmap_clear(&map);
    assert(map.size == 0);

    /* 3. High-Density Collision Stress Test (600 keys into 257 buckets -> guaranteed collisions) */
    char key[24];
    for (size_t i = 0; i < 600; ++i) {
        snprintf(key, sizeof(key), "STK%04zu", i);
        assert(hashmap_put(&map, key, i * 3) == 1);
    }
    assert(map.size == 600);
    assert(map.collision_count > 300); /* Pigeonhole principle: >= 600 - 257 = 343 collisions */
    assert(hashmap_load_factor(&map) > 2.0);

    for (size_t i = 0; i < 600; ++i) {
        snprintf(key, sizeof(key), "stk%04zu", i);
        size_t steps = 0;
        assert(hashmap_get_with_steps(&map, key, &val, &steps) == 1);
        assert(val == i * 3);
        assert(steps >= 1);
    }

    /* Remove half of the colliding keys and verify remaininghalf are intact */
    for (size_t i = 0; i < 600; i += 2) {
        snprintf(key, sizeof(key), "STK%04zu", i);
        assert(hashmap_remove(&map, key) == 1);
    }
    assert(map.size == 300);
    for (size_t i = 1; i < 600; i += 2) {
        snprintf(key, sizeof(key), "STK%04zu", i);
        assert(hashmap_get(&map, key, &val) == 1 && val == i * 3);
    }
    hashmap_clear(&map);

    /* 4. Compare Linear, Binary, Hash, and AVL Searches */
    Portfolio p;
    portfolio_init(&p);
    portfolio_seed_sample_data(&p);

    SearchComparisonReport rep;
    search_compare_all(&p.stocks, &p.ticker_index, &p.avl_index, "SUNPHARMA", &rep);
    assert(rep.linear_metric.found == 1);
    assert(rep.binary_metric.found == 1);
    assert(rep.hash_metric.found == 1);
    assert(rep.avl_metric.found == 1);
    assert(rep.linear_metric.index == rep.hash_metric.index);
    assert(rep.binary_metric.index == rep.avl_metric.index);

    /* Substring search test */
    size_t matches[16];
    size_t mcount = search_linear_substring(&p.stocks, "Bank", matches, 16);
    assert(mcount == 2); /* HDFCBANK and ICICIBANK */

    portfolio_destroy(&p);
    printf("[PASS] All Hash Table & Searching Algorithm tests passed!\n");
    return 0;
}
