#ifndef HASH_H
#define HASH_H

typedef unsigned long hash_t;

typedef struct {
    char *key;
    void *value;
} HashPair;

typedef struct {
    char *key;
    void *value;
    struct HashEntry *next;
    hash_t hash;
} HashEntry;

typedef struct {
    struct HashEntryChunk *next;
    int size;
} HashEntryChunk;

typedef struct {
    HashEntry       **bucket;
    HashEntryChunk  *freelist;
    HashEntry       *iter_entry;
    int             buckets;        /* hash_function(string_key) % buckets */
    int             count;
    int             cap;
    int             iter_bucket;
} HashTable;



HashTable*  hash_new();
bool        hash_alloc  (HashTable *ht, int buckets, int cap);
void        hash_free   (HashTable *ht);

bool        hash_store  (HashTable *ht, char *key, void *value);
void*       hash_fetch  (HashTable *ht, char *key);
void        hash_delete (HashTable *ht, char *key);
bool        hash_exists (HashTable *ht, char *key);
int         hash_count  (HashTable *ht);
HashPair*   hash_exists_pair (HashTable *ht, char *key);

void        hash_iter_reset  (HashTable *ht);
char*       hash_iter_keys   (HashTable *ht);
void*       hash_iter_values (HashTable *ht);
HashPair*   hash_iter_pairs  (HashTable *ht);

void        hash_dump   (HashTable *ht);






#endif
