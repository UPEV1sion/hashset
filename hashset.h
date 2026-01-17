#ifndef HASHSET_H
#define HASHSET_H

#ifndef HSDEF
/*
    If your code is a single source file, and you want to get rid of unused functions, you can `#define HSDEF static inline`,
    to let the compiler remove the unused functions. 
*/
#define HSDEF 
#endif // HSDEF

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *item;
    size_t item_size;
    size_t hash;
} hashset_bucket_t;

typedef struct {
    hashset_bucket_t **buckets;
    size_t cap;
    size_t used;
} hashset_t;

HSDEF int hashset_insert(hashset_t *hs, void *item, size_t item_size);
HSDEF int hashset_remove(hashset_t *hs, void *item, size_t item_size);
HSDEF size_t hashset_size(hashset_t *hs);
HSDEF bool hashset_contains(hashset_t *hs, void *item, size_t item_size);
HSDEF void hashset_free(hashset_t *hs);

#ifdef HASHSET_IMPLEMENTATION

#define max(a, b) (((a) > (b)) ? (a) : (b)) 
#define HS_ASSERT(a, ...) \
    do { \
        if(!(a)) { \
             fprintf(stderr, __VA_ARGS__); \
             exit(1); \
        } \
    }while(0) 

#ifndef HASHSET_INIT_CAP
#define HASHSET_INIT_CAP 1024
#endif // HASHSET_INIT_CAP

#ifndef HASHSET_LOAD_FACTOR
#define HASHSET_LOAD_FACTOR 0.75
#endif // HASHSET_LOAD_FACTOR

#ifndef HASHSET_GROW_RATE
#define HASHSET_GROW_RATE 2
#endif // HASHSET_GROW_RATE

#ifndef hashset_hash
#define hashset_hash hashset__hash
#endif // hashset_hash

#ifndef hashset_equal
#define hashset_equal hashset__equal
#endif // hashset_equal

HSDEF size_t hashset__hash(void *item, size_t item_size)
{
    static const size_t magic_prime = 0x00000100000001b3;
    size_t hash = 0xcbf29ce484222325;
    
    uint8_t *bytes = item;
    while(item_size-- > 0) 
        hash = (hash ^ *bytes++) * magic_prime;

    return hash;
}

HSDEF bool hashset__equal(void *a, size_t a_size, void *b, size_t b_size)
{
    if(a_size != b_size) return false;
    return memcmp(a, b, a_size) == 0;
}

HSDEF hashset_bucket_t* hashset__bucket_new(void *item, const size_t item_size, const size_t hash)
{
    hashset_bucket_t *bucket = malloc(sizeof(hashset_bucket_t));
    if(!bucket) return NULL;
    bucket->item = malloc(item_size);
    if(!bucket->item)
    {
        free(bucket);
        return NULL;
    }
    memcpy(bucket->item, item, item_size);
    bucket->item_size = item_size;
    bucket->hash = hash;    
    return bucket;
}

HSDEF void hashset__bucket_free(hashset_bucket_t *bucket)
{
    free(bucket->item);
    free(bucket);
}

HSDEF size_t hashset__find_free_bucket(hashset_bucket_t **buckets, size_t cap, size_t hash)
{
    while(buckets[hash] != NULL) hash = (hash + 1) % cap;
    return hash;
}

HSDEF void hashset__rehash(hashset_t *hs, size_t new_cap, hashset_bucket_t **new_buckets)
{
    if(hs->cap == 0) return;

    for(size_t i = 0; i < hs->cap; ++i)
    {
        hashset_bucket_t *cur_bucket = hs->buckets[i];
        if(!cur_bucket) continue;
        
        size_t new_pos = hashset__find_free_bucket(new_buckets, new_cap, cur_bucket->hash % new_cap);
        new_buckets[new_pos] = cur_bucket;
    }
}

HSDEF int hashset__grow(hashset_t *hs)
{
    const size_t new_cap = max(HASHSET_INIT_CAP, hs->cap * HASHSET_GROW_RATE);
    hashset_bucket_t **new_buckets = calloc(new_cap, sizeof(hashset_bucket_t*));
    if(!new_buckets) return -1;
    hashset__rehash(hs, new_cap, new_buckets);   
    hs->cap = new_cap; 
    free(hs->buckets);
    hs->buckets = new_buckets;
    return 0;
}

HSDEF int hashset_insert(hashset_t *hs, void *item, size_t item_size)
{
    HS_ASSERT(hs, "Hashset can not be NULL!\n");
    
    if(hs->used >= hs->cap * HASHSET_LOAD_FACTOR) 
        if(hashset__grow(hs) != 0) return -1;

    const size_t raw_hash = hashset_hash(item, item_size);
    size_t hash = raw_hash % hs->cap;

    size_t start = hash;
    while(hs->buckets[hash] != NULL)
    {
        hashset_bucket_t *b = hs->buckets[hash];
        if(hashset_equal(b->item, b->item_size, item, item_size)) return 0;
        hash = (hash + 1) % hs->cap;
        if(hash == start) return -1;
    }

    hashset_bucket_t *bucket = hashset__bucket_new(item, item_size, raw_hash);
    if(!bucket) return -1;

    hs->buckets[hash] = bucket;
    hs->used++;
    return 0;

    return 0;
}

HSDEF int hashset_remove(hashset_t *hs, void *item, size_t item_size)
{
    HS_ASSERT(hs, "Hashset can not be NULL!\n");
    HS_ASSERT(hs->cap > 0, "Hashset can not be empty!\n");    

    size_t hash = hashset_hash(item, item_size) % hs->cap;
    size_t start = hash;

    while(hs->buckets[hash] != NULL)
    {
        hashset_bucket_t *b = hs->buckets[hash];
        if(hashset_equal(b->item, b->item_size, item, item_size)) break;
        
        hash = (hash + 1) % hs->cap;
        if(hash == start) return -1;
    }
     
    if(hs->buckets[hash] == NULL) return -1;
    hashset__bucket_free(hs->buckets[hash]);
    hs->buckets[hash] = NULL;
    hs->used--;

    size_t next = (hash + 1) % hs->cap;
    while(hs->buckets[next] != NULL)
    {
        hashset_bucket_t *b = hs->buckets[next];
        hs->buckets[next] = NULL;
        size_t new_pos = b->hash % hs->cap;
        while(hs->buckets[new_pos] != NULL) new_pos = (new_pos + 1) % hs->cap;
        hs->buckets[new_pos] = b;

        next = (next + 1) % hs->cap;
    }

    return 0;
}

HSDEF size_t hashset_size(hashset_t *hs)
{
    HS_ASSERT(hs, "Hashset can not be NULL!\n");
    HS_ASSERT(hs->cap > 0, "Hashset can not be empty!\n");    

    return hs->used;
}

HSDEF bool hashset_contains(hashset_t *hs, void *item, size_t item_size)
{
    HS_ASSERT(hs, "Hashset can not be NULL!\n");
    HS_ASSERT(hs->cap > 0, "Hashset can not be empty!\n");    

    size_t hash = hashset_hash(item, item_size) % hs->cap;
    while(hs->buckets[hash] != NULL)
    {
        hashset_bucket_t *b = hs->buckets[hash];
        if(hashset_equal(b->item, b->item_size, item, item_size)) return true;
        hash = (hash + 1) % hs->cap;
    }

    return false;
}

HSDEF void hashset_free(hashset_t *hs)
{
    HS_ASSERT(hs, "Hashset can not be NULL!\n");

    for(size_t i = 0; i < hs->cap; ++i)
    {   
        hashset_bucket_t *cur_bucket = hs->buckets[i];
        if(!cur_bucket) continue;
        hashset__bucket_free(cur_bucket);
   }
    free(hs->buckets);
}

#endif // HASHSET_IMPLEMENTATION
#endif // HASHSET_H

/*
    Revision history:
        1.0.0 (2026-01-17): first release
*/
