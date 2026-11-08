#include "hashmap.h"
#include "stock.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Hash Table with Separate Chaining
 * Uses DJB2 case-insensitive hash over normalized ticker symbols.
 * Average time complexity: O(1) insert, lookup, delete.
 */

unsigned long hashmap_hash(const char *key) {
    unsigned long h = 5381;
    if (!key) return 0;
    while (*key) {
        h = ((h << 5) + h) + (unsigned char)toupper((unsigned char)*key++);
    }
    return h % HASH_CAPACITY;
}

void hashmap_init(HashMap *map) {
    if (!map) return;
    memset(map, 0, sizeof(*map));
}

int hashmap_put(HashMap *map, const char *key, size_t value) {
    if (!map || !key || !*key) return 0;
    unsigned long bucket = hashmap_hash(key);
    HashNode *node = map->buckets[bucket];
    while (node) {
        if (stock_stricmp(node->key, key) == 0) {
            node->value = value;
            return 1;
        }
        node = node->next;
    }

    HashNode *new_node = (HashNode *)calloc(1, sizeof(HashNode));
    if (!new_node) return 0;
    stock_normalize_ticker(key, new_node->key, sizeof(new_node->key));
    new_node->value = value;
    if (map->buckets[bucket] != NULL) {
        map->collision_count++;
    }
    new_node->next = map->buckets[bucket];
    map->buckets[bucket] = new_node;
    map->size++;
    return 1;
}

int hashmap_get_with_steps(const HashMap *map, const char *key, size_t *value, size_t *steps_out) {
    size_t steps = 1;
    if (!map || !key || !*key) {
        if (steps_out) *steps_out = 0;
        return 0;
    }
    unsigned long bucket = hashmap_hash(key);
    HashNode *node = map->buckets[bucket];
    while (node) {
        if (stock_stricmp(node->key, key) == 0) {
            if (value) *value = node->value;
            if (steps_out) *steps_out = steps;
            return 1;
        }
        steps++;
        node = node->next;
    }
    if (steps_out) *steps_out = steps;
    return 0;
}

int hashmap_get(const HashMap *map, const char *key, size_t *value) {
    return hashmap_get_with_steps(map, key, value, NULL);
}

int hashmap_remove(HashMap *map, const char *key) {
    if (!map || !key || !*key) return 0;
    unsigned long bucket = hashmap_hash(key);
    HashNode *curr = map->buckets[bucket];
    HashNode *prev = NULL;
    while (curr) {
        if (stock_stricmp(curr->key, key) == 0) {
            if (prev) prev->next = curr->next;
            else map->buckets[bucket] = curr->next;
            free(curr);
            if (map->size > 0) map->size--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

double hashmap_load_factor(const HashMap *map) {
    if (!map) return 0.0;
    return (double)map->size / (double)HASH_CAPACITY;
}

void hashmap_clear(HashMap *map) {
    if (!map) return;
    for (size_t i = 0; i < HASH_CAPACITY; ++i) {
        HashNode *node = map->buckets[i];
        while (node) {
            HashNode *next = node->next;
            free(node);
            node = next;
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
    map->collision_count = 0;
}
