#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hash.h"

int main(int argc, char **argv) {

    HashTable *hash = hash_new();

    hash_store(hash, "key", "value");
    printf("value = '%s'\n", (char *) hash_fetch(hash, "key"));

    hash_store(hash, "key_one", "value1");
    hash_store(hash, "key_two", "value2");
    hash_store(hash, "key_three", "value3");
    hash_dump(hash);

    hash_store(hash, "key_four", "value4");
    hash_store(hash, "key_five", "value5");
    hash_dump(hash);

    printf("exists 'key_three' ? %d\n", hash_exists(hash, "key_three"));
    printf("exists 'key_thre' ? %d\n", hash_exists(hash, "key_thre"));
    printf("hash table (%d entries / %d buckets)\n", hash_count(hash), hash->cap);
    puts("");

    hash_delete(hash, "key_one");
    hash_delete(hash, "key_four");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_two");
    hash_delete(hash, "key_three");
    hash_delete(hash, "key_five");
    hash_dump(hash);



    hash_free(hash);
    free(hash);

    return 0;
}

