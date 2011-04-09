/*
 * Copyright 2011 Mathias Brossard <mathias@brossard.org>
 */
/**
 * @file has.c
 */

#include "has.h"

#include <stdlib.h>
#include <string.h>

#define hash_size(s) ((s) + ((s) >> 1))
#define hash_freed ((void *)1)
#define hash_hash(d, i) (d->value.hash.hash[i])
#define hash_first(d, h) (h % hash_size(d->value.hash.size))
#define hash_next(d, i) (((i + 1) == hash_size(d->value.hash.size)) ? 0 : i + 1)

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
            if(e->value.hash.entries[i].key.pointer) {
                if(e->value.hash.entries[i].key.owner) {
                    free(e->value.hash.entries[i].key.pointer);
                }
                if(e->value.hash.entries[i].value) {
                    has_free(e->value.hash.entries[i].value);
                }
            }
        }
        free(e->value.hash.entries);
        free(e->value.hash.hash);
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

void has_set_owner(has_t *e, bool owner)
{
    if(e) {
        e->owner = owner ? 1 : 0;
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
            has_hash_entry_t *l = &(e->value.hash.entries[i]);
            if(l->key.pointer) {
                WF(r, f(e, has_walk_hash_key, j, l->key.pointer,
                        l->key.size, NULL, p));
                WF(r, f(e, has_walk_hash_value_begin, j, NULL, 0, l->value, p));
                WF(r, has_walk(l->value, f, p));
                WF(r, f(e, has_walk_hash_value_end, j, NULL, 0, l->value, p));
                j++;
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
    has_hash_entry_t *e, **h;

    if(hash == NULL ||
       ((e = calloc(sizeof(has_hash_entry_t), size)) == NULL) ||
       ((h = calloc(sizeof(has_hash_entry_t *), hash_size(size))) == NULL)) {
        return NULL;
    }

    hash->type = has_hash;
    hash->value.hash.size = size;
    hash->value.hash.count = 0;
    hash->value.hash.entries = e;
    hash->value.hash.hash = h;
    return hash;
}

inline bool has_is_hash(has_t *e)
{
    return (e && e->type == has_hash) ? true : false;
}

has_t * has_hash_set_o(has_t *hash, char *key, size_t size, has_t *value, bool owner)
{
    size_t            i, j;
    has_hash_entry_t *e = NULL;
    int32_t           h;

    if(hash == NULL || hash->type != has_hash) {
        return NULL;
    }

    h = has_hash_function(key, size);
    /* Search for a value with same key */

    for(i = hash_first(hash, h); (e = hash_hash(hash, i)) ; i = hash_next(hash, i)) {
        if((e != hash_freed) &&                        /* Check freed */
           (e->hash == h) &&                           /* Check hash */
           (e->key.size == size) &&                    /* Check key size */
           (memcmp(e->key.pointer, key, size) == 0)) { /* Full key compare */
            has_free(e->value);       /* Free the value */
            e->value = value;
            if(e->key.owner) {
                free(e->key.pointer); /* Free the key if we own it */
            }
            e->key.pointer = key;
            e->key.owner = owner;
            return hash;
        }
    }

    if(hash->value.hash.size == hash->value.hash.count) {
        has_hash_entry_t **t;
        i = hash->value.hash.size * sizeof(has_hash_entry_t);
        if((e = calloc(2* i, 1)) == NULL) {
            return NULL;
        }
        memcpy(e, hash->value.hash.entries, i);
        free(hash->value.hash.entries);
        hash->value.hash.entries = e;

        i = 2 * hash->value.hash.size;
        if((t = calloc(hash_size(i), sizeof(has_hash_entry_t *))) == NULL) {
            return NULL;
        }
        free(hash->value.hash.hash);
        hash->value.hash.hash = t;

        hash->value.hash.size *= 2;

        /* Rebuild hash table */
        for(i = 0 ; i < hash->value.hash.count ; i++) {
            e = &(hash->value.hash.entries[i]);
            j = hash_first(hash, e->hash);
            while(hash_hash(hash, j) != NULL && hash_hash(hash, j) != hash_freed) {
                j = hash_next(hash, j);
            }
            hash_hash(hash, j) = e;
        }
    }

    /* Insert key in the first empty slot. Start at
       hash->value.hash.count and wrap at
       hash->value.hash.size. Because hash->value.hash.count <
       hash->value.hash.size this will necessarily terminate. */
    for (i = hash->value.hash.count ; hash->value.hash.entries[i].key.pointer ; ) {
        i = (i + 1 == hash->value.hash.size) ? 0 : i + 1;
    }

    /* Insert element */
    e = &(hash->value.hash.entries[i]);
    e->key.pointer = key;
    e->key.size = size;
    e->key.owner = owner;
    e->hash = h;
    e->value = value;

    j = hash_first(hash, e->hash);
    while(hash_hash(hash, j) != NULL && hash_hash(hash, j) != hash_freed) {
        j = hash_next(hash, j);
    }
    hash_hash(hash, j) = e;

    /* Increase counter */
    hash->value.hash.count++;
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

has_t *has_hash_add(has_t *h, has_t *k, has_t *v)
{
    if(h == NULL || h->type != has_hash ||
       k == NULL || k->type != has_string) {
        return NULL;
    } else {
        has_t *r;
        char *s = k->value.string.pointer;
        size_t l = k->value.string.size;
        bool owner = k->value.string.owner;
        if(owner) {
            k->value.string.owner = false;
        }
        has_free(k);
        if((r = has_hash_set_o(h, s, l, v, owner)) == NULL) {
            has_free(v);
            if(owner) {
                free(s);
            }
        }
        return r;
    }
}

bool has_hash_exists(has_t *hash, const char *key, size_t size)
{
    size_t            i;
    uint32_t          h;
    has_hash_entry_t *e = NULL;
    bool              r = false;

    if(!hash) {
        return r;
    }

    h = has_hash_function(key, size);
    for(i = hash_first(hash, h); (e = hash_hash(hash, i)) ; i = hash_next(hash, i)) {
        if((e != hash_freed) &&                        /* Check freed */
           (e->hash == h) &&                           /* Check hash */
           (e->key.size == size) &&                    /* Check key size */
           (memcmp(e->key.pointer, key, size) == 0)) { /* Full key compare */
            r = true;
            break;
        }
    }

    return r;
}

bool has_hash_exists_str(has_t *hash, const char *string)
{
    return has_hash_exists(hash, string, strlen(string));
}

has_t * has_hash_get(has_t *hash, const char *key, size_t size)
{
    size_t            i;
    uint32_t          h;
    has_hash_entry_t *e = NULL;
    has_t            *r = NULL;

    if(!hash) {
        return r;
    }

    h = has_hash_function(key, size);
    for(i = hash_first(hash, h); (e = hash_hash(hash, i)) ; i = hash_next(hash, i)) {
        if((e != hash_freed) &&                        /* Check freed */
           (e->hash == h) &&                           /* Check hash */
           (e->key.size == size) &&                    /* Check key size */
           (memcmp(e->key.pointer, key, size) == 0)) { /* Full key compare */
            r = e->value;
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
    size_t            i;
    has_hash_entry_t *e;
    has_t            *r = NULL;
    uint32_t          h;

    if(hash == NULL) {
        return NULL;
    }

    h = has_hash_function(key, size);
    for(i = hash_first(hash, h); (e = hash_hash(hash, i)) ; i = hash_next(hash, i)) {
        if((e != hash_freed) &&                        /* Check freed */
           (e->hash == h) &&                           /* Check hash */
           (e->key.size == size) &&                    /* Check key size */
           (memcmp(e->key.pointer, key, size) == 0)) { /* Full key compare */
            if(e->key.owner) {
                free(e->key.pointer);
            }
            r = e->value;

            /* Lazy free */
            hash->value.hash.hash[i] = hash_freed;
            hash->value.hash.count--;
            e->hash = 0;
            e->key.size = 0;
            e->key.pointer = NULL;
            break;
        }
    }

    /* Resilver when hash is empty */
    if(hash->value.hash.count == 0) {
        memset(hash->value.hash.entries, 0,
               hash->value.hash.size * sizeof(has_hash_entry_t));
        memset(hash->value.hash.hash, 0,
               hash_size(hash->value.hash.size) * sizeof(has_hash_entry_t *));
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

int has_hash_keys_values(has_t *hash, char ***keys, size_t **lengths,
                         has_t ***values, int *count)
{
    char  **k = NULL;
    size_t *l = NULL;
    has_t  **v = NULL;
    int     i, j;

    if(hash == NULL || hash->type != has_hash ||
       (keys == NULL && lengths == NULL && values == NULL) ||
       ((keys == NULL) != (lengths == NULL)) ||
       ((keys && lengths) &&
        ((k = calloc(sizeof(char *), (hash->value.hash.count + 1))) == NULL ||
         (l = calloc(sizeof(size_t), (hash->value.hash.count + 1))) == NULL)) ||
       ((values != NULL) &&
        (v = calloc(sizeof(has_t *), (hash->value.hash.count + 1))) == NULL)) {
        return -1;
    }

    for(i = 0, j = 0; j < hash->value.hash.count &&
            i < hash->value.hash.size; i++) {
        if(hash->value.hash.entries[i].key.pointer) {
            if(k && l) {
                k[j] = hash->value.hash.entries[i].key.pointer;
                l[j] = hash->value.hash.entries[i].key.size;
            }
            if(v) {
                v[j] = hash->value.hash.entries[i].value;
            }
            j++;
        }
    }
    if(count) {
        *count = hash->value.hash.count;
    }

    if(keys && lengths) {
        *keys = k;
        *lengths = l;
    }
    if(values) {
        *values = v;
    }
    return 0;
}

int has_hash_keys(has_t *hash, char ***keys, size_t** lengths, int *count)
{
    return has_hash_keys_values(hash, keys, lengths, NULL, count);
}

int has_hash_values(has_t *hash, has_t ***values, int *count)
{
    return has_hash_keys_values(hash, NULL, NULL, values, count);
}

static char *xstrndup(const char *str, int l)
{
    char *r = NULL;
    if(l >= 0 && (r = calloc(l + 1, 1)) != NULL) {
        memcpy(r, str, l);
    }
    return r;
}

int has_hash_keys_str(has_t *hash, char ***keys, int *count)
{
    char **k;
    int i, j;

    if(hash == NULL || hash->type != has_hash || keys == NULL ||
       (k = calloc(sizeof(char *), (hash->value.hash.count + 1))) == NULL) {
        return -1;
    }

    for(i = 0, j = 0; j < hash->value.hash.count &&
            i < hash->value.hash.size; i++) {
        if(hash->value.hash.entries[i].key.pointer) {
            if((k[j] = xstrndup(hash->value.hash.entries[i].key.pointer,
                                hash->value.hash.entries[i].key.size)) == NULL) {
                break;
            }
            j++;
        }
    }

    if(j != hash->value.hash.count) {
        for(i = 0 ; k[i]; i++) {
            free(k[i]);
        }
        free(k);
        return -1;
    }

    if(count) {
        *count = hash->value.hash.count;
    }
    *keys = k;
    return 0;
}

has_t * has_array_new(size_t size)
{
    has_t *r = has_new(1), *s = NULL;
    if(r && (s = has_array_init(r, size)) == NULL) {
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
    return (array && array->type == has_array &&
            array->value.array.count > index) ?
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

inline bool has_is_string(has_t *e)
{
    return (e && e->type == has_string) ? true : false;
}

has_t * has_null_new(int32_t value)
{
    return has_null_init(has_new(1));
}

has_t * has_null_init(has_t *null)
{
    if(null) {
        null->type = has_null;
    }
    return null;
}

inline bool has_is_null(has_t *e)
{
    return (e && e->type == has_null) ? true : false;
}

has_t * has_int_new(int32_t value)
{
    return has_int_init(has_new(1), value);
}

has_t * has_int_init(has_t *integer, int32_t value)
{
    if(integer) {
        integer->type = has_integer;
        integer->value.integer = value;
    }
    return integer;
}

inline bool has_is_int(has_t *e)
{
    return (e && e->type == has_integer) ? true : false;
}

int32_t has_int_get(has_t *integer)
{
    return (integer && integer->type == has_integer) ?  
        integer->value.integer : 0;
}

has_t * has_bool_new(bool value)
{
    return has_bool_init(has_new(1), value);
}

has_t * has_bool_init(has_t *boolean, bool value)
{
    if(boolean) {
        boolean->type = has_boolean;
        boolean->value.boolean = value;
    }
    return boolean;
}

bool has_bool_get(has_t *boolean)
{
    return (boolean && boolean->type == has_boolean) ?  
        boolean->value.boolean : 0;
}


has_t * has_double_new(double value)
{
    return has_double_init(has_new(1), value);
}

has_t * has_double_init(has_t *fp, double value)
{
    if(fp) {
        fp->type = has_double;
        fp->value.fp = value;
    }
    return fp;
}

double has_double_get(has_t *fp)
{
    return (fp && fp->type == has_double) ?  
        fp->value.fp : 0.0;
}

inline bool has_is_array(has_t *e)
{
    return (e && e->type == has_array) ? true : false;
}

inline bool has_is_boolean(has_t *e)
{
    return (e && e->type == has_boolean) ? true : false;
}

inline bool has_is_double(has_t *e)
{
    return (e && e->type == has_double) ? true : false;
}

inline bool has_is_pointer(has_t *e)
{
    return (e && e->type == has_pointer) ? true : false;
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
