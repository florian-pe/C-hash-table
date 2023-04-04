#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "hash.h"



/* DJB2 */
hash_t hash_compute(char *string)
{
    hash_t hash = 5381;
    int c;

    while ((c = *string++) != 0) {
        hash = ((hash << 5) + hash) + ((unsigned char) c);  /* hash * 33 + c */
    }

    return hash;
}


HashTable *hash_new()
{
    HashTable *ht;
    
    ht = malloc(sizeof (HashTable));
    if (!ht) {
        return NULL;
    }

    memset(ht, 0, sizeof (HashTable));

    if (!hash_alloc(ht, 16, 32)) {
        return NULL;
    }
    return ht;
}

/* bytes of padding necessary to reach the $align alignment boundary */
#define ALIGNMENT_PADDING(size, align) ((align - (size % align)) % align)

bool hash_alloc(HashTable *ht, int buckets, int cap)
{
    HashEntry **bucket;
    size_t size_bucket_array;
    size_t padding;

    if (buckets > cap) {
        cap = buckets;
    }

    size_bucket_array = (sizeof (HashEntry *)) * buckets;
    padding = ALIGNMENT_PADDING(size_bucket_array, 8);

    bucket = malloc(size_bucket_array + padding + (sizeof (HashEntry)) * cap);
    if (!bucket) {
        return 0;
    }

    memset(bucket, 0, size_bucket_array);

    ht->bucket      = bucket;
    ht->buckets     = buckets;
    ht->cap         = cap;
    ht->count       = 0;
    ht->iter_entry  = NULL;

    ht->freelist = (HashEntryChunk *) (((char *) bucket) + size_bucket_array + padding);
    ht->freelist->next = NULL;
    ht->freelist->size = cap;

    return 1;
}

void hash_free(HashTable *ht) {
    free(ht->bucket);
}

static
bool hash_resize(HashTable *ht, int buckets, int cap)
{
    HashTable new_ht;
    HashEntry *entry;
    int i;

    if (!hash_alloc(&new_ht, buckets, cap)) {
        return 0;
    }

    for (i=0; i < ht->buckets; i++)
    {
        for (entry = ht->bucket[i]; entry; entry = (HashEntry *) entry->next)
        {
            if (!hash_store(&new_ht, entry->key, entry->value)) {
                return 0;
            }
        }
    }

    free(ht->bucket);
    *ht = new_ht;

    return 1;
}


static
HashEntry *hash_freelist_alloc_entry(HashTable *ht)
{
    HashEntryChunk *chunk;

    chunk = ht->freelist;

    if (chunk->size > 1) {

        ht->freelist = (HashEntryChunk *) (((char *) ht->freelist) + (sizeof (HashEntry)));
        ht->freelist->next = chunk->next;
        ht->freelist->size = chunk->size - 1;

        return (HashEntry *) chunk;
    }

    ht->freelist = (HashEntryChunk *) ht->freelist->next;

    return (HashEntry *) chunk;
}

static
void hash_freelist_free_entry(HashTable *ht, HashEntry *entry)
{
    ((HashEntryChunk *) entry)->next = ht->freelist->next;
    ht->freelist = (HashEntryChunk *) entry;
}

bool hash_store(HashTable *ht, char *key, void *value)
{
    HashEntry *entry;
    hash_t hash;
    int slot;

    hash = hash_compute(key);

SEARCH_ENTRY:
    slot = hash % ht->buckets;

    /* empty chain */
    if (!ht->bucket[slot]) {
        goto ALLOCATE_ENTRY;
    }

    /* non-empty chain */
    entry = ht->bucket[slot];
    while (entry) {

        /* non-empty chain and there is an existing entry with the same key */
        if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
            entry->value = value;
            return 1;
        }
        
        entry = (HashEntry *) entry->next;
    }

    /* when reaching this point:  */
    /* non-empty entry chain and all the entries have a different key than the one we want to insert */

ALLOCATE_ENTRY:
    /* check if space left */
    if (!ht->freelist) {
        /* try to allocate more space and rehash the table */
        if (!hash_resize(ht, ht->buckets * 2, ht->cap * 2)) {
            return 0;
        }
        /* the hashtable has been rehashed, need to start searching again
         * since the slot number of the key to insert will be different in general
         *
         * this going back can happen only once, because the second time there will be enough space
         * since we just successfully increase the available space, therefore no infinte loop
         * */
        goto SEARCH_ENTRY;
    }

    entry = hash_freelist_alloc_entry(ht);

    entry->key = key;
    entry->value = value;
    entry->hash = hash;
    entry->next = (struct HashEntry *) ht->bucket[slot];

    ht->bucket[slot] = entry;
    ht->count++;

    return 1;
}


void *hash_fetch(HashTable *ht, char *key)
{
    HashEntry *entry;
    hash_t hash;

    hash = hash_compute(key);

    for (entry = ht->bucket[hash % ht->buckets]; entry; entry = (HashEntry *) entry->next)
    {
        if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
            return entry->value;
        }
    }

    return NULL;
}

void hash_delete(HashTable *ht, char *key)
{
    HashEntry *entry;
    HashEntry *prev;
    hash_t hash;
    int slot;

    hash = hash_compute(key);
    slot = hash % ht->buckets;
    entry = ht->bucket[slot];

    if (!entry) {
        return;
    }
    if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
        ht->bucket[slot] = (HashEntry *) entry->next;
        ht->count--;
        hash_freelist_free_entry(ht, entry);
        return;
    }

    prev = entry;
    entry = (HashEntry *) entry->next;

    while (entry) {

        if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
            prev->next = entry->next;
            ht->count--;
            hash_freelist_free_entry(ht, entry);
            return;
        }

        prev = entry;
        entry = (HashEntry *) entry->next;
    }
}

bool hash_exists(HashTable *ht, char *key)
{
    HashEntry *entry;
    hash_t hash;

    hash = hash_compute(key);

    for (entry = ht->bucket[hash % ht->buckets]; entry; entry = (HashEntry *) entry->next)
    {
        if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
            return 1;
        }
    }

    return 0;
}

HashPair* hash_exists_pair(HashTable *ht, char *key)
{
    HashEntry *entry;
    hash_t hash;

    hash = hash_compute(key);

    for (entry = ht->bucket[hash % ht->buckets]; entry; entry = (HashEntry *) entry->next)
    {
        if (entry->hash == hash && (strcmp(entry->key, key) == 0)) {
            return (HashPair *) entry;
        }
    }

    return NULL;
}

int hash_count(HashTable *ht) {
    return ht->count;
}

void hash_iter_reset(HashTable *ht) {
    ht->iter_entry  = NULL;
}

char *hash_iter_keys(HashTable *ht)
{
    HashPair *pair = hash_iter_pairs(ht);
    if (pair) {
        return pair->key;
    }
    return NULL;
}

void *hash_iter_values(HashTable *ht)
{
    HashPair *pair = hash_iter_pairs(ht);
    if (pair) {
        return pair->value;
    }
    return NULL;
}

HashPair *hash_iter_pairs(HashTable *ht)
{
    HashEntry *entry;
    int i;

    if (ht->iter_entry) {
        i = ht->iter_bucket;
        entry = ht->iter_entry;
        goto NEXT_ENTRY;
    }

    for (i=0; i < ht->buckets; i++)
    {
        entry = ht->bucket[i];
        
        while (entry)
        {
            ht->iter_bucket = i;
            ht->iter_entry  = entry;
            return (HashPair *) entry;

            NEXT_ENTRY:
            entry = (HashEntry *) entry->next;
        }
    }

    ht->iter_entry = NULL;
    return NULL;
}

void hash_dump(HashTable *ht)
{
    HashEntry *entry;
    int i;

    for (i=0; i < ht->buckets; i++)
    {
        printf("[%d]", i);
        for (entry = ht->bucket[i]; entry; entry = (HashEntry *) entry->next)
        {
            printf("->(%s => %p)", entry->key, entry->value);
        }
        printf("->NULL\n");
    }
    printf("-----------------------------------------\n");
}


