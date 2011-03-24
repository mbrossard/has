/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#ifndef _HAS_H
#define	_HAS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    has_zero = 0,
    has_hash,
    has_array,
    has_string,
    has_boolean,
    has_integer,
    has_double,
    has_pointer
} has_types;

typedef struct _has_t has_t;

typedef struct {
    has_t **elements;
    size_t size;
    size_t count;
} has_array_t;

typedef struct _has_hash_list_t {
    uint32_t hash;
    char *key;
    size_t size;
    has_t *value;
    struct _has_hash_list_t *next;
    bool owner;
} has_hash_list_t;

typedef struct {
    has_hash_list_t **elements;
    size_t size;
    size_t count;
} has_hash_t;

typedef struct {
    char *pointer;
    size_t size;
    bool owner;
} has_string_t;

struct _has_t {
    union {
        has_array_t array;
        has_hash_t hash;
        has_string_t string;
        uint32_t uint;
        int32_t integer;
        double fp;
        bool boolean;
        void *pointer;
    } value;
    unsigned char type;
    bool owner;
};

/** */
has_t * has_new(size_t count);
void has_free(has_t *e);
void has_set_owner(has_t *e, bool owner);

has_t * has_hash_new(size_t size);
has_t * has_hash_init(has_t *hash, size_t size);
has_t * has_hash_set(has_t *hash, char *key, size_t size, has_t *value);
has_t * has_hash_set_o(has_t *hash, char *key, size_t size, has_t *value, bool owner);
has_t * has_hash_set_str(has_t *hash, char *string, has_t *value);
has_t * has_hash_set_str_o(has_t *hash, char *string, has_t *value, bool owner);
has_t * has_hash_get(has_t *hash, const char *key, size_t size);
has_t * has_hash_get_str(has_t *hash, const char *string);
has_t * has_hash_remove(has_t *hash, const char *key, size_t size);
has_t * has_hash_remove_str(has_t *hash, const char *string);
bool has_hash_delete(has_t *hash, const char *key, size_t size);
bool has_hash_delete_str(has_t *hash, const char *string);
int has_hash_count(has_t *hash);
int has_hash_keys(has_t *hash, char **keys, size_t* lengths, int *count);
int has_hash_keys_values(has_t *hash, char **keys, size_t* lengths,
                         has_t **values, int *count);

has_t * has_array_new(size_t size);
has_t * has_array_init(has_t *array, size_t size);
has_t * has_array_push(has_t *array, has_t *value);
has_t * has_array_pop(has_t *a);
has_t * has_array_unshift(has_t *array, has_t *value);
has_t * has_array_shift(has_t *a);
has_t * has_array_set(has_t *array, size_t index, has_t *value);
has_t * has_array_get(has_t *array, size_t index);
int has_array_count(has_t *array);

has_t * has_string_new(char *pointer, size_t size);
has_t * has_string_new_o(char *pointer, size_t size, bool owner);
has_t * has_string_new_str(char *string);
has_t * has_string_new_str_o(char *string, bool owner);

has_t * has_string_init(has_t *string, char *pointer, size_t size, bool owner);
has_t * has_string_init_str(has_t *string, char *str, bool owner);

has_t * has_int_new(int32_t value);
has_t * has_int_init(has_t *integer, int32_t value);
has_t * has_uint_new(uint32_t value);
has_t * has_uint_init(has_t *integer, uint32_t value);

#ifdef __cplusplus
};
#endif

#endif /* _HAS_H */
