#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

#define HASH_CAPACITY 257

typedef struct HashNode {
    char key[24];
    size_t value;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_CAPACITY];
    size_t size;
    size_t collision_count;
} HashMap;

/* Legacy alias for compatibility */
typedef HashMap HashTable;

void hashmap_init(HashMap *map);
unsigned long hashmap_hash(const char *key);
int hashmap_put(HashMap *map, const char *key, size_t value);
int hashmap_get(const HashMap *map, const char *key, size_t *value);
int hashmap_get_with_steps(const HashMap *map, const char *key, size_t *value, size_t *steps_out);
int hashmap_remove(HashMap *map, const char *key);
double hashmap_load_factor(const HashMap *map);
void hashmap_clear(HashMap *map);

#endif /* HASHMAP_H */
