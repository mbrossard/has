/*
 * Copyright 2016 Mathias Brossard <mathias@brossard.org>
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

/**
 * @struct has_string_t
 * @brief String Structure
 */
typedef struct {
    /** Pointer to string content */
    char     *pointer;
    /** Size of string */
    size_t    size;
    /** Flag specifying is pointer should be freed */
    bool      owner;
} has_string_t;

/**
 * @struct has_array_t
 * @brief Array Structure
 */
typedef struct {
    /** Array of pointers to elements */
    has_t     **elements;
    /** Number of allocated slots for elements */
    size_t      size;
    /** Number of elements present  */
    size_t      count;
} has_array_t;

/**
 * @struct has_hash_entry_t
 * @brief Associative Array Sub-structure
 */
typedef struct  {
    /** Digest of key */
    uint32_t      hash;
    /** Pointer to hash entry value */
    has_t        *value;
    /** Hash entry key */
    has_string_t  key;
} has_hash_entry_t;

/**
 * @struct has_hash_t
 * @brief Associative Array Structure
 */
typedef struct {
    /** Array of hash entries */
    has_hash_entry_t  *entries;
    /** Open-addressing array of entries pointers */
    has_hash_entry_t **hash;
    /** Number of allocated slots for entries */
    size_t             size;
    /** Number of entries present */
    size_t             count;
} has_hash_t;

/**
 * @struct has_value_t
 * @brief has_t value Union
 */
typedef union {
    /** Array */
    has_array_t array;
    /** Associative Array */
    has_hash_t hash;
    /** String */
    has_string_t string;
    /** Unsigned Integer */
    uint32_t uint;
    /** Signed Integer */
    int32_t integer;
    /** Floating Pointer */
    double fp;
    /** Boolean */
    bool boolean;
    /** Pointer */
    void *pointer;
} has_value_t;

struct has_t {
    /** Value of has_t element */
    has_value_t value;
    /** Type of has_t element @see has_types */
    unsigned char type;
    /** Flag specifying if has_t element can be deallocated */
    bool owner;
};

/**
 * @typedef has_walk_function_t
 * @brief Callback function type for has_walk()
 * @see has_walk has_walk_t
 * @param [in] cur     Current element
 * @param [in] type    Type of callback (see #has_walk_t)
 * @param [in] index   Index of the element (if relevant)
 * @param [in] string  Pointer to string (if relevant)
 * @param [in] size    Size of string (if relevant)
 * @param [in] element Pointer to a sub-element (if relevant)
 * @param [in] pointer Pointer passed to has_walk()
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
 */

/**
 * @brief Allocates and initializes a hash has_t structure.
 * @param [in] size Initial size of the hash
 */
has_t * has_hash_new(size_t size);

/**
 * @brief Initializes a hash has_t structure.
 * @param [in] hash Pointer to hash has_t element to initialize
 * @param [in] size Initial size of the hash
 */
has_t * has_hash_init(has_t *hash, size_t size);

/**
 * @brief Tests if a has_t structure is a hash.
 * @param [in] hash Pointer to hash has_t element to test.
 * @return @c true if pointer is not null and has_t element is a hash,
 * @c false otherwise.
 */
bool has_is_hash(has_t *hash);

/**
 * @brief Retrieves the number of elements in a hash has_t structure.
 * @param [in] hash Pointer to hash has_t element
 */
int has_hash_count(has_t *hash);

/**
 * @brief Adds an element to hash.
 * @param [in] hash Pointer to hash has_t element to which add the
 * value.
 * @param [in] key   Pointer to the key.
 * @param [in] size  Size of the key.
 * @param [in] value Pointer to has_t element containing the value.
 * @return hash if successful or @c NULL if has is not defined, is not a
 * hash, or if memory allocation failed.
 */
has_t * has_hash_set(has_t *hash, char *key, size_t size, has_t *value);

/**
 * @brief Adds an element to hash with key ownership
 * @param [in] hash  Pointer to hash has_t element to which add the value.
 * @param [in] key   Pointer to the key.
 * @param [in] size  Size of the key.
 * @param [in] value Pointer to has_t element containing the value.
 * @param [in] owner Boolean value specifying the ownership of the key.
 * @return hash if successful or @c NULL if has is not defined, is not a
 * hash, or if memory allocation failed.
 */
has_t * has_hash_set_o(has_t *hash, char *key, size_t size,
                       has_t *value, bool owner);

/**
 * @brief Adds an element to hash with key as string
 * @param [in] hash   Pointer to hash has_t element to which add the value.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string to be
 * used as key
 * @param [in] value  Pointer to has_t element containing the value.
 * @return hash if successful or @c NULL if has is not defined, is not a
 * hash, or if memory allocation failed.
 */
has_t * has_hash_set_str(has_t *hash, char *string, has_t *value);

/**
 * @brief Adds an element to hash with key as string and ownership
 * @param [in] hash   Pointer to hash has_t element to which add the value.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string to be
 * used as key.
 * @param [in] value  Pointer to has_t element containing the value.
 * @param [in] owner  Boolean value specifying the ownership of the key.
 * @return hash if successful or @c NULL if has is not defined, is not a
 * hash, or if memory allocation failed.
 */
has_t * has_hash_set_str_o(has_t *hash, char *string,
                           has_t *value, bool owner);

/**
 * @brief Adds an element to hash.
 * @param [in] hash Pointer to hash has_t element to which add the
 * value.
 * @param [in] key   Pointer to has_t element containing the key.
 * @param [in] value Pointer to has_t element containing the value.
 * @return hash if successful or @c NULL if has is not defined, is not a
 * hash, or if memory allocation failed.
 *
 * The key has_t element will be freed.
 */
has_t * has_hash_add(has_t *hash, has_t *key, has_t *value);

/**
 * @brief Determines if an entry with matching key exists in hash.
 * @param [in] hash  Pointer to hash has_t element to test.
 * @param [in] key   Pointer to the key.
 * @param [in] size  Size of the key.
 * @return true if the key exists in hash element, false if not found
 * or if hash is @c NULL or not a has_hash element.
 */
bool has_hash_exists(has_t *hash, const char *key, size_t size);

/**
 * @brief Determines if an entry with matching key (passed as string)
 * exists in hash.
 * @param [in] hash   Pointer to hash has_t element to test.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @return true if the key exists in hash element, false if not found
 * or if hash is @c NULL or not a has_hash element.
 */
bool has_hash_exists_str(has_t *hash, const char *string);

/**
 * @brief Retrieves an entry from hash based on its key.
 * @param [in] hash  Pointer to hash has_t element to test.
 * @param [in] key   Pointer to the key.
 * @param [in] size  Size of the key.
 * @return the value corresponding to the key, @c NULL if not found or
 * if hash is @c NULL or not a has_hash element.
 */
has_t * has_hash_get(has_t *hash, const char *key, size_t size);

/**
 * @brief Retrieves an entry from hash based on its key (passed as
 * string).
 * @param [in] hash  Pointer to hash has_t element to test.
 * @param [in] key   Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @return the value corresponding to the key, @c NULL if not found or
 * if hash is @c NULL or not a has_hash element.
 */
has_t * has_hash_get_str(has_t *hash, const char *key);

/**
 * @brief Removes an entry from hash based on its key.
 * @param [in] hash  Pointer to hash has_t element to test.
 * @param [in] key   Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @param [in] size  Size of the key.
 * @return the value corresponding to the key, @c NULL if not found or
 * if hash is @c NULL or not a has_hash element.
 */
has_t * has_hash_remove(has_t *hash, const char *key, size_t size);

/**
 * @brief Removes an entry from hash based on its key (passed as
 * string).
 * @param [in] hash Pointer to hash has_t element to test.
 * @param [in] key  Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @return the value corresponding to the key, @c NULL if not found or
 * if hash is @c NULL or not a has_hash element.
 */
has_t * has_hash_remove_str(has_t *hash, const char *key);

/**
 * @brief Deletes an entry from hash based on its key.
 * @param [in] hash Pointer to hash has_t element to test.
 * @param [in] key  Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @param [in] size  Size of the key.
 * @return @c true if found and @c false if not found or
 * if hash is @c NULL or not a has_hash element.
 */
bool has_hash_delete(has_t *hash, const char *key, size_t size);

/**
 * @brief Deletes an entry from hash based on its key (passed as
 * string).
 * @param [in] hash  Pointer to hash has_t element to test.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string
 * containing the key.
 * @return @c true if found and @c false if not found or
 * if hash is @c NULL or not a has_hash element.
 */
bool has_hash_delete_str(has_t *hash, const char *string);

/**
 * @brief Retrieves hash keys pointers and lengthes in C arrays
 */
int has_hash_keys(has_t *hash, char ***keys, size_t** lengths, int *count);

/**
 * @brief Retrieves hash values in C array
 */
int has_hash_values(has_t *hash, has_t ***values, int *count);

/**
 * @brief Retrieves hash content in C arrays
 */
int has_hash_keys_values(has_t *hash, char ***keys, size_t** lengths,
                         has_t ***values, int *count);

/**
 * @brief Retrieves hash keys as <tt>NULL</tt>-terminated strings
 */
int has_hash_keys_str(has_t *hash, char ***keys, int *count);

/** @} */

/**
 * @defgroup array Array functions
 * @{
 */

/**
 * @brief Allocates and initializes an array has_t element.
 * @param [in] size Initial size of the hash
 * @return Pointer to array has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_array_new(size_t size);

/**
 * @brief Initializes an array has_t element.
 * @param [in] array  Pointer to array has_t element to initialize.
 * @param [in] size Initial size of the hash
 * @return Pointer to array has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_array_init(has_t *array, size_t size);

/**
 * @brief Tests if a has_t element is an array.
 * @param [in] array  Pointer to array has_t
 * @return @c true if pointer is not null and has_t element is an
 * array, @c false otherwise.
 */
bool has_is_array(has_t *array);

/**
 * @brief Retrieves the number of elements in an array has_t
 * element.
 * @param [in] array  Pointer to array has_t.
 */
int has_array_count(has_t *array);

/**
 * @brief Adds an element at the end of a has_t array.
 * @param [in] array  Pointer to array has_t.
 * @param [in] value  Pointer to has_t value.
 * @return Pointer to array has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_array_push(has_t *array, has_t *value);

/**
 * @brief Retrieves an element from the end of a has_t array.
 * @param [in] array  Pointer to array has_t
 * @return Pointer to has_t element retrieved if successful, @c NULL
 * otherwise.
 */
has_t * has_array_pop(has_t *array);

/**
 * @brief Adds an element at the start of a has_t array.
 * @param [in] array  Pointer to array has_t
 * @param [in] value  Pointer to has_t value.
 * @return Pointer to array has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_array_unshift(has_t *array, has_t *value);

/**
 * @brief Retrieves an element from the start of a has_t array.
 * @param [in] array  Pointer to array has_t.
 * @return Pointer to has_t element retrieved if successful, @c NULL
 * otherwise.
 */
has_t * has_array_shift(has_t *array);

/**
 * @brief Sets an element at given postion of a has_t array.
 * @param [in] array  Pointer to array has_t.
 * @param [in] index  Index at which set the value.
 * @param [in] value  Pointer to has_t value.
 * @return Pointer to array has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_array_set(has_t *array, size_t index, has_t *value);

/**
 * @brief Retrieves an element at given postion of a has_t array.
 * @param [in] array  Pointer to array has_t.
 * @param [in] index  Index at which retrieve the value.
 * @return Pointer to has_t element retrieved if successful, @c NULL
 * otherwise.
 */
has_t * has_array_get(has_t *array, size_t index);

/** @} */

/**
 * @defgroup string String functions
 * Function related to string has_t elements.
 * @{
 */

/**
 * @brief Allocates and initializes a string has_t element.
 * @param [in] string Pointer to string.
 * @param [in] size   Size of string.
 * @return Pointer to string has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_string_new(char *string, size_t size);

/**
 * @brief Allocates and initializes a string has_t element with
 * ownership.
 * @param [in] string Pointer to string.
 * @param [in] size   Size of string.
 * @param [in] owner  Boolean value specifying the ownership of the
 * string.
 * @return Pointer to string has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_string_new_o(char *string, size_t size, bool owner);

/**
 * @brief Allocates and initializes a string has_t element with
 * string.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string.
 * @return Pointer to string has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_string_new_str(char *string);

/**
 * @brief Allocates and initializes a string has_t element with string
 * and owner ship.
 * @param [in] string Pointer to <tt>NULL</tt>-terminated string.
 * @param [in] owner  Boolean value specifying the ownership of the string.
 * @return Pointer to string has_t element if successful, @c NULL
 * otherwise.
 */
has_t * has_string_new_str_o(char *string, bool owner);

/**
 * @brief Initializes a string has_t element with ownership.
 * @param [in] string  Pointer to has_t element to initialize.
 * @param [in] pointer Pointer to string.
 * @param [in] size    Size of string.
 * @param [in] owner   Boolean value specifying the ownership of the
 * string.
 * @return string if successful, @c NULL otherwise.
 */
has_t * has_string_init(has_t *string, char *pointer, size_t size, bool owner);

/**
 * @brief Initializes a string has_t element with string and
 * ownership.
 * @param [in] string  Pointer to has_t element to initialize.
 * @param [in] pointer Pointer to <tt>NULL</tt>-terminated string.
 * @param [in] owner   Boolean value specifying the ownership of the
 * string.
 * @return string if successful, @c NULL otherwise.
 */
has_t * has_string_init_str(has_t *string, char *pointer, bool owner);

/**
 * @brief Tests if a has_t element is a string.
 * @param [in] string  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is a
 * string, @c false otherwise.
 */
bool has_is_string(has_t *string);

/** @} */

/**
 * @brief Allocates and initializes a null has_t element.
 */
has_t * has_null_new();

/**
 * @brief Initializes a null has_t element.
 */
has_t * has_null_init(has_t *null);

/**
 * @brief Tests if a has_t element is a null.
 * @param [in] e  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is a
 * a null, @c false otherwise.
 */
bool has_is_null(has_t *e);

/**
 * @brief Allocates and initializes an integer has_t element.
 */
has_t * has_int_new(int32_t value);

/**
 * @brief Initializes an integer has_t element.
 */
has_t * has_int_init(has_t *integer, int32_t value);

/**
 * @brief Tests if a has_t element is an integer.
 * @param [in] e  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is an
 * integer, @c false otherwise.
 */
bool has_is_int(has_t *e);

/* FIXME */
int32_t has_int_get(has_t *integer);

/**
 * @brief Allocates and initializes a boolean has_t element.
 */
has_t * has_bool_new(bool value);

/**
 * @brief Initializes a boolean has_t element.
 */
has_t * has_bool_init(has_t *boolean, bool value);

/**
 * @brief Tests if a has_t element is a boolean.
 * @param [in] e  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is a
 * boolean, @c false otherwise.
 */
bool has_is_boolean(has_t *e);

/* FIXME */
bool has_bool_get(has_t *boolean);

/**
 * @brief Allocates and initializes a double has_t element.
 */
has_t * has_double_new(double value);

/**
 * @brief Initializes a double has_t element.
 */
has_t * has_double_init(has_t *fp, double value);

/**
 * @brief Tests if a has_t element is a double.
 * @param [in] e  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is a
 * double, @c false otherwise.
 */
bool has_is_double(has_t *e);

/* FIXME */
double has_double_get(has_t *fp);

/**
 * @brief Allocates and initializes a pointer has_t element.
 */
has_t * has_pointer_new(void *value);

/**
 * @brief Initializes a pointer has_t element.
 */
has_t * has_pointer_init(has_t *pointer, void *value);

/**
 * @brief Tests if a has_t element is a pointer.
 * @param [in] e  Pointer to has_t element to test.
 * @return @c true if pointer is not null and has_t element is a
 * pointer, @c false otherwise.
 */
bool has_is_pointer(has_t *e);

uint32_t has_hash_function(const char * data, int len);

#ifdef __cplusplus
};
#endif

#endif /* _HAS_H */
