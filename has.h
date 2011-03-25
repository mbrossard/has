/*
 * Copyright 2011 Mathias Brossard <mathias@brossard.org>
 */
/**
 * @file has.h
 */

#ifndef _HAS_H
#define	_HAS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum has_types
 * @brief List of constants for the different element types
 */
typedef enum {
    has_null = 0, /** Unitialized       */
    has_hash,     /** Associative array */
    has_array,    /** Array             */
    has_string,   /** String            */
    has_integer,  /** Integer           */
    has_boolean,  /** Boolean           */
    has_double,   /** Floating point    */
    has_pointer   /** Pointer           */
} has_types;

/**
 * @enum has_walk_t
 * @brief List of constant passed to the callback function by
 * has_walk().
 */
typedef enum {
    /** Called when entering a hash element */
    has_walk_hash_begin,
    /** Called for each key of a hash: index, string and pointer
        passed to callback function */
    has_walk_hash_key,
    /** Called for each value of a hash before traversal: index and
        element passed to callback function */
    has_walk_hash_value_begin,
    /** Called for each value of a hash after traversal: index and
        element passed to callback function */
    has_walk_hash_value_end,
    /** Called when leaving a hash element */
    has_walk_hash_end,
    /** Called when entering an array element */
    has_walk_array_begin,
    /** Called for each entry of an array before traversal: index and
        element passed to callback function */
    has_walk_array_entry_begin,
    /** Called for each entry of an array after traversal: index and
        element passed to callback function */
    has_walk_array_entry_end,
    /** Called when leaving an array element */
    has_walk_array_end,
    /** Called for each string: string and pointer passed to callback
        function */
    has_walk_string,
    /** Called for all elements except strings, arrays and hashes:
        string and pointer passed to callback function */    
    has_walk_other
} has_walk_t;

/* Forward declarations */
/**
 * @struct has_t
 * @brief Main structure
 */
typedef struct has_t has_t;
typedef struct has_hash_list_t has_hash_list_t;

/**
 * @struct has_string_t
 * @brief String Structure
 */
typedef struct {
    char *pointer;
    size_t size;
    bool owner;
} has_string_t;

/**
 * @struct has_array_t
 * @brief Array Structure
 */
typedef struct {
    has_t **elements;
    size_t size;
    size_t count;
} has_array_t;

/**
 * @struct has_hash_t
 * @brief Associative Array Structure
 */
typedef struct {
    has_hash_list_t **elements;
    size_t size;
    size_t count;
} has_hash_t;

struct has_t {
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

/**
 * @struct has_hash_list_t
 * @brief Associative Array Sub-structure
 */
struct has_hash_list_t {
    uint32_t hash;
    char *key;
    size_t size;
    has_t *value;
    has_hash_list_t *next;
    bool owner;
};

/** @typedef has_walk_function_t
 *  @brief Callback function type for has_walk()
 *  @see has_walk has_walk_t
 *  @param [in] cur     Current element
 *  @param [in] type    Type of callback (see #has_walk_t)
 *  @param [in] index   Index of the element (if relevant)
 *  @param [in] string  Pointer to string (if relevant)
 *  @param [in] size    Size of string (if relevant)
 *  @param [in] element Pointer to a sub-element (if relevant)
 *  @param [in] pointer Pointer passed to has_walk()
 */
typedef int (*has_walk_function_t)(has_t *cur, has_walk_t type, int index,
                                   const char *string, size_t size,
                                   has_t *element, void *pointer);

/**
 * @brief Allocates one or several has_t element(s)
 * @param count
 * @return A pointer to one or several has_t element(s) or @c NULL.
 */
has_t * has_new(size_t count);

/**
 * @brief Frees the content of a has_t structure
 * @param  e    Pointer to has_t structure
 * @return void
 */
void has_free(has_t *e);

/**
 * @brief Affect ownership of has_t structure
 * @param  e     Pointer to has_t structure
 * @param  owner New value for ownership property
 * @return void
 */
void has_set_owner(has_t *e, bool owner);

/**
 * @brief Calls callback function during traversal of a has_t
 * structure.
 */
int has_walk(has_t *e, has_walk_function_t f, void *p);

/**
 * @defgroup hash Associative array functions
 * @{
 * @brief Allocates and initializes a hash has_t structure.
 */
has_t * has_hash_new(size_t size);

/**
 * @brief Initializes a hash has_t structure.
 */
has_t * has_hash_init(has_t *hash, size_t size);

/**
 * @brief Tests if a has_t structure is a hash.
 */
bool has_is_hash(has_t *e);

/**
 * @brief Retrieves the number of elements in a hash has_t structure.
 */
int has_hash_count(has_t *hash);

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

int has_hash_keys(has_t *hash, char **keys, size_t* lengths, int *count);
int has_hash_keys_values(has_t *hash, char **keys, size_t* lengths,
                         has_t **values, int *count);
/** @} */

/**
 * @defgroup array Array functions
 * @{
 * @brief Allocates and initializes an array has_t element.
 */
has_t * has_array_new(size_t size);

/**
 * @brief Initializes an array has_t element.
 */
has_t * has_array_init(has_t *array, size_t size);

/**
 * @brief Tests if a has_t element is an array.
 */
bool has_is_array(has_t *e);

/**
 * @brief Retrieves the number of elements in an array has_t
 * element.
 */
int has_array_count(has_t *array);

has_t * has_array_push(has_t *array, has_t *value);
has_t * has_array_pop(has_t *a);

has_t * has_array_unshift(has_t *array, has_t *value);
has_t * has_array_shift(has_t *a);

has_t * has_array_set(has_t *array, size_t index, has_t *value);
has_t * has_array_get(has_t *array, size_t index);

/** @} */

/**
 * @defgroup string String functions
 * Function related to string has_t elements.
 * @{
 * @brief Allocates and initializes a string has_t element.
 */
has_t * has_string_new(char *pointer, size_t size);
has_t * has_string_new_o(char *pointer, size_t size, bool owner);
has_t * has_string_new_str(char *string);
has_t * has_string_new_str_o(char *string, bool owner);

has_t * has_string_init(has_t *string, char *pointer, size_t size, bool owner);
has_t * has_string_init_str(has_t *string, char *str, bool owner);

/**
 * @brief Tests if a has_t element is a string.
 */
bool has_is_string(has_t *e);

/** @} */

has_t * has_null_new();
has_t * has_null_init(has_t *null);
bool has_is_null(has_t *e);

has_t * has_int_new(int32_t value);
has_t * has_int_init(has_t *integer, int32_t value);

/**
 * @brief Tests if a has_t element is an integer.
 */
bool has_is_int(has_t *e);
int32_t has_int_get(has_t *integer);

has_t * has_bool_new(bool value);
has_t * has_bool_init(has_t *boolean, bool value);

/**
 * @brief Tests if a has_t element is a boolean.
 */
bool has_is_boolean(has_t *e);
bool has_bool_get(has_t *boolean);

has_t * has_double_new(double value);
has_t * has_double_init(has_t *fp, double value);

/**
 * @brief Tests if a has_t element is a double.
 */
bool has_is_double(has_t *e);
double has_double_get(has_t *fp);

bool has_is_pointer(has_t *e);

uint32_t has_hash_function(const char * data, int len);

#ifdef __cplusplus
};
#endif

#endif /* _HAS_H */
