/*
 * Copyright 2011 Mathias Brossard <mathias@brossard.org>
 */
/**
 * @file has.c
 */

#include "has.h"

#include <stdlib.h>
#include <string.h>

has_t * has_new(size_t count)
{
    has_t *r = calloc(sizeof(has_t), count);
    if(r && count == 1) {
        r->owner = 1;
    }
    return r;
}

void has_free(has_t *e)
{
    if(e == NULL) {
        return;
    }

    if(e->type == has_hash) {
        int i;
        for(i = 0; i < e->value.hash.size; i++) {
            has_hash_list_t *f, *g;
            for(f = e->value.hash.elements[i]; f;) {
                if(f->owner) {
                    free(f->key);
                }
                if(f->value) {
                    has_free(f->value);
                }
                g = f;
                f = f->next;
                free(g);
            }
        }
        free(e->value.hash.elements);
    } else if(e->type == has_array) {
        int i;
        for(i = 0; i < e->value.array.count; i++) {
            has_free(e->value.array.elements[i]);
        }
        free(e->value.array.elements);
    } else if(e->type == has_string && 
              e->value.string.owner) {
        free(e->value.string.pointer);
    }
    if(e->owner) {
        free(e);
    }
}

/* I don't like to use macros to often */
#define WF(r, call) if((r = (call)) != 0) { return r; } 

int has_walk(has_t *e, has_walk_function_t f, void *p)
{
    int i, j, r;

    if(e == NULL) {
        return -1;
    }

    if(e->type == has_hash) {
        WF(r, f(e, has_walk_hash_begin, 0, NULL, 0, NULL, p));
        for(i = 0, j = 0; i < e->value.hash.size; i++) {
            has_hash_list_t *l;
            for(l = e->value.hash.elements[i]; l; l = l->next, j++) {
                WF(r, f(e, has_walk_hash_key, j, l->key, l->size, NULL, p));
                WF(r, f(e, has_walk_hash_value_begin, j, NULL, 0, l->value, p));
                WF(r, has_walk(l->value, f, p));
                WF(r, f(e, has_walk_hash_value_end, j, NULL, 0, l->value, p));
            }
        }
        WF(r, f(e, has_walk_hash_end, 0, NULL, 0, NULL, p));
    } else if(e->type == has_array) {
        WF(r, f(e, has_walk_array_begin, 0, NULL, 0, NULL, p));
        for(i = 0; i < e->value.array.count; i++) {
            has_t *cur = e->value.array.elements[i];
            WF(r, f(e, has_walk_array_entry_begin, i, NULL, 0, cur, p));
            WF(r, has_walk(cur, f, p));
            WF(r, f(e, has_walk_array_entry_end, i, NULL, 0, cur, p));
        }
        WF(r, f(e, has_walk_array_end, 0, NULL, 0, NULL, p));
    } else if(e->type == has_string) {
        WF(r, f(e, has_walk_string, 0, e->value.string.pointer,
             e->value.string.size, NULL, p));
    } else {
        WF(r, f(e, has_walk_other, 0, NULL, 0, NULL, p));
    }
    return 0;
}
#undef WF

void has_set_owner(has_t *e, bool owner)
{
    if(e) {
        e->owner = owner ? 1 : 0;
    }
}

has_t * has_hash_new(size_t size)
{
    has_t *r = has_new(1), *s = NULL;
    if(r) {
        if((s = has_hash_init(r, size)) == NULL) {
            has_free(r);
        }
    }
    return s;
}

has_t * has_hash_init(has_t *hash, size_t size)
{
    has_hash_list_t **tmp;
    if(hash == NULL ||
       (tmp = calloc(sizeof(has_hash_list_t *), size)) == NULL) {
        return NULL;
    }

    hash->type = has_hash;
    hash->value.hash.size = size;
    hash->value.hash.count = 0;
    hash->value.hash.elements = tmp;
    return hash;
}

has_t * has_hash_set_o(has_t *hash, char *key, size_t size, has_t *value, bool owner)
{
    has_hash_list_t *l, *cur, *prev;

    if(hash == NULL || (l = calloc(sizeof(has_hash_list_t), 1)) == NULL) {
        return NULL;
    }

    l->key = key;
    l->size = size;
    l->hash = has_hash_function(key, size);
    l->owner = owner;
    l->value = value;

    /* Insert in front */
    l->next = hash->value.hash.elements[ l->hash % hash->value.hash.size ];
    hash->value.hash.elements[ l->hash % hash->value.hash.size ] = l;

    /* Search for a value with same key */
    for(prev = l, cur = l->next; cur; prev = cur, cur = cur->next) {
        if((cur->hash == l->hash) &&                 /* Check hash */
           (cur->size == l->size) &&                 /* Check key size */
           memcmp(cur->key, l->key, l->size) == 0) { /* Compare keys */
            /* Found a previous value with same key */
            prev->next = cur->next; /* Remove it from hash link */
            has_free(cur->value);   /* Free the value */
            if(cur->owner) {
                free(cur->key);     /* Free the key if we own it */
            }
            free(cur);              /* Free the hash link entry */
            break;
        }
    }
    return hash;
}

has_t * has_hash_set(has_t *hash, char *key, size_t size, has_t *value)
{
    return has_hash_set_o(hash, key, size, value, false);
}

has_t * has_hash_set_str(has_t *hash, char *string, has_t *value)
{
    return has_hash_set_o(hash, string, strlen(string), value, false);
}

has_t * has_hash_set_str_o(has_t *hash, char *string, has_t *value, bool owner)
{
    return has_hash_set_o(hash, string, strlen(string), value, owner);
}

has_t * has_hash_get(has_t *hash, const char *key, size_t size)
{
    has_hash_list_t *cur;
    has_t *r = NULL;
    uint32_t h;
    if(hash == NULL) {
        return NULL;
    }

    h = has_hash_function(key, size);
    for (cur = hash->value.hash.elements[ h % hash->value.hash.size ];
         cur ; cur = cur->next) {
        if((h == cur->hash) &&                 /* Check hash */
           (size == cur->size) &&              /* Check key size */
           memcmp(key, cur->key, size) == 0) { /* Compare keys */
            r = cur->value;                    /* Found match */
            break;
        }
    }
    return r;
}

has_t * has_hash_get_str(has_t *hash, const char *string)
{
    return has_hash_get(hash, string, strlen(string));
}

has_t * has_hash_remove(has_t *hash, const char *key, size_t size)
{
    has_hash_list_t *cur, **prev;
    has_t *r = NULL;
    uint32_t h;

    if(hash == NULL) {
        return NULL;
    }

    h = has_hash_function(key, size);
    prev = &(hash->value.hash.elements[ h % hash->value.hash.size ]);
    for (cur = *prev; cur ; prev = &(cur->next), cur = cur->next) {
        if((h == cur->hash) &&                 /* Check hash */
           (size == cur->size) &&              /* Check key size */
           memcmp(key, cur->key, size) == 0) { /* Compare keys */
            /* Found entry */
            r = cur->value;        /* Keep track of value */
            (*prev) = cur->next;   /* Remove from hash link */
            if(cur->owner) {
                free(cur->key);    /* Free the key if we own it */
            } 
            free(cur);             /* Free the hash link entry */
            break;
        }
    }
    return r;
}

has_t * has_hash_remove_str(has_t *hash, const char *string)
{
    return has_hash_remove(hash, string, strlen(string));
}

bool has_hash_delete(has_t *hash, const char *key, size_t size)
{
    has_t *t = has_hash_remove(hash, key, size);
    if(t) {
        has_free(t);
        return true;
    }
    return false;
}

bool has_hash_delete_str(has_t *hash, const char *string)
{
    return has_hash_delete(hash, string, strlen(string));
}

has_t * has_array_new(size_t size)
{
    has_t *r = has_new(1), *s = NULL;
    if((s = has_array_init(r, size)) == NULL) {
        has_free(r);
    }
    return s;
}

has_t * has_array_init(has_t *array, size_t size)
{
    if(array == NULL) {
        return NULL;
    }

    array->value.array.elements = calloc(sizeof(has_t *), size);
    if(array->value.array.elements == NULL) {
        return NULL;
    }
    array->type = has_array;
    array->value.array.size = size;
    array->value.array.count = 0;
    return array;
}

has_t * has_array_reallocate(has_t *array, size_t size)
{
    size_t n;
    has_t **new;

    if(array == NULL || array->type != has_array) {
        return NULL;
    }

    if((n = array->value.array.size) > size) {
        return array; /* Already big enough */
    }

    while(n < size) { n *= 2; } /* Double until big enough */
    new = realloc(array->value.array.elements, n * sizeof(has_t *));
    if(new == NULL) {
        return NULL;
    }

    /* Zero new part */
    memset(new + sizeof(has_t *) * array->value.array.size, 0,
           sizeof(has_t *) * (n - array->value.array.size));

    array->value.array.elements = new;
    array->value.array.size = n;

    return array;
}

has_t * has_array_push(has_t *array, has_t *value)
{
    if(array == NULL) {
        return NULL;
    }

    if(array->value.array.count == array->value.array.size &&
       (has_array_reallocate(array, array->value.array.size * 2) == NULL)) {
        return NULL;
    }
    array->value.array.elements[array->value.array.count] = value;
    array->value.array.count++;
    return array;
}

has_t * has_array_pop(has_t *array)
{
    has_t *r = NULL;
    if(array && array->type == has_array && array->value.array.count > 0) {
        array->value.array.count--;
        r = array->value.array.elements[array->value.array.count];
        array->value.array.elements[array->value.array.count] = NULL;
    }
    return r;
}

has_t * has_array_set(has_t *array, size_t index, has_t *value)
{
    if(array == NULL || array->type != has_array) {
        return NULL;
    }

    if(index > array->value.array.size &&
       (has_array_reallocate(array, index) == NULL)) {
        return NULL;
    }

    array->value.array.elements[index] = value;
    if(index >= array->value.array.count) {
        array->value.array.count = index + 1;
    }
    return array;
}

has_t * has_array_get(has_t *array, size_t index)
{
    return (array && array->type == has_array && array->value.array.count > index) ?
        array->value.array.elements[index] : NULL;
}

int has_array_count(has_t *array)
{
    return (array && array->type == has_array) ? array->value.array.count : 0;
}

has_t * has_string_init(has_t *string, char *pointer, size_t size, bool owner)
{
    if(string == NULL) {
        return NULL;
    }

    string->type = has_string;
    string->value.string.pointer = pointer;
    string->value.string.owner = owner;
    string->value.string.size = size;
    return string;
}

has_t * has_string_init_str(has_t *string, char *str, bool owner)
{
    return has_string_init(string, str, strlen(str), owner);
}

has_t * has_string_new(char *pointer, size_t size)
{
    return has_string_init(has_new(1), pointer, size, false);
}

has_t * has_string_new_o(char *pointer, size_t size, bool owner)
{
    return has_string_init(has_new(1), pointer, size, owner);
}

has_t * has_string_new_str(char *str)
{
    return has_string_init(has_new(1), str, strlen(str), false);
}

has_t * has_string_new_str_o(char *str, bool owner)
{
    return has_string_init(has_new(1), str, strlen(str), owner);
}

has_t * has_int_new(int32_t value)
{
    return has_int_init(has_new(1), value);
}

has_t * has_int_init(has_t *integer, int32_t value)
{
    if(integer) {
        integer->type = has_string;
        integer->value.integer = value;
    }
    return integer;
}

has_t * has_uint_new(uint32_t value)
{
    return has_uint_init(has_new(1), value);
}

has_t * has_uint_init(has_t *integer, uint32_t value)
{
    if(integer) {
        integer->type = has_string;
        integer->value.uint = value;
    }
    return integer;
}

#define SuperFastHash has_hash_function

/*
 * SuperFastHash by Paul Hsieh : http://www.azillionmonkeys.com/qed/hash.html
 * Public domain version http://burtleburtle.net/bob/hash/doobs.html
 */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((const uint8_t *)(d))[1] << UINT32_C(8))\
                      +((const uint8_t *)(d))[0])
#endif

uint32_t SuperFastHash (const char * data, int len) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
