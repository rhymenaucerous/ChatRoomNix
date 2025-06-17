#ifndef H_TABLE_LIB
#define H_TABLE_LIB

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "../cll_lib/cll.h"
#include "../algorithms_lib/algorithms.h"

#ifndef SHARED_MACROS
#define SHARED_MACROS

#define FREE(a) \
    free(a); \
    (a) = NULL

//macros enabling clear returns from all functions.
#define SUCCESS 0
#define FAILURE 1
#define FAILURE_NEGATIVE -1

//NOTE: 512, 1024, 4096 are common buffer size chunks due to them being powers
//of 2, efficiently using memory. 1024 is a nice middle ground for potential
//packet sizes.
#define BUFF_SIZE 1024

#define CONTINUE 1
#define STOP 0

//filenames should not exceed 50 characters - this will enable some paths.
#define FILE_NAME_MAX_LEN 50

//Maximums for strings: IPv4 or IPv6 compatable.
#define HOST_MAX_STRING 40 //Max IP length is (IPv6) 40 -> 
                           //7 colons + 32 hexadecimal digits + terminator.

#define PORT_MAX_STRING 6 //Only numeric services allowed - max length of
                          //65535 is 5 + terminator.

//IP length + Port length - 1 (one less terminator) + 2 (: designator in code).
#define ADDR_MAX_STRING (HOST_MAX_STRING + PORT_MAX_STRING + 1)

#endif //SHARED_MACROS

#ifndef KEY_LENGTH

#define KEY_LENGTH 10

#endif

/**
 * @brief Notes on hash table library.
 *
 * Key length is set via macro at 5. This value was selected to plan for a
 * smaller hash table. Set via macro to abstract functionality from user and
 * enable quicker/easier use of library. User-supplied p_key is cast to a char
 * pointer before being cast to int pointer to hash calulcations.
 *
 * The hash table capacity and size are both uint16_t type. This intentionally
 * limits the size of the hash table and save memory. The maximum size of the
 * hash table is 65535 but the re-hashing function only increases to the next
 * prime number so unless a higher number is given on initialization, the
 * largest size the hash table will reach is 65521.
 * 
 */

/**
 * @brief Entry structure for hash table.
 * 
 * @param p_key pointer to the user supplied key.
 * @param p_data pointer to user-supplied data for entry.
 * @param p_free_function pointer to user supplied free function.
 */
typedef struct entry_t {
    const void * p_key;
    void * p_data;
    void (*p_free_function) (void *);
} entry_t;

/**
 * @brief HAsh table context structure.
 * 
 * @param size number of entries in the hash table.
 * @param capacity maximum capacity of the hash table.
 * @param p_hash_function pointer to hash function.
 * @param pp_array pointer to array of clls. This functionality enables
 * chaining at each array index.
 */
typedef struct h_table_t {
    uint16_t size;
    uint16_t capacity;
    int64_t (*p_hash_function) (const void *);
    cll_t ** pp_array;
} h_table_t;

/**
 * @brief Initializes hash table context. Sets hash function and initial size
 * and capacity.
 * 
 * @param capacity The user-supplied initial capacity of the hash table.
 * @param p_hash_function user-supplied function for hashing the keys. accepts
 * a const void and returns an int.
 * @return h_table_t* pointer to hash table context structure.
 */
h_table_t * h_table_init (uint16_t capacity, 
                          int64_t (*p_hash_function) (const void *));

/**
 * @brief Function that adds new entries to the hash table.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_data user supplied data to be entered into hash table.
 * @param p_key pointer to user-supplied key.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
int h_table_new_entry (h_table_t * p_h_table, void * p_data,
                                        const void * p_key);

/**
 * @brief Returns an entry from the hash table given a key.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_key pointer to user-supplied key.
 * @return void* pointer to specified data. Will return NULL if function fails.
 */
void * h_table_return_entry (h_table_t * p_h_table, void * p_key);

/**
 * @brief Destroys an entry in the hash table given a key.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_key pointer to user-supplied key.
 * @return void* pointer to specified data. Returns NULL if function fails.
 */
void * h_table_destroy_entry (h_table_t * p_h_table, void * p_key);

/**
 * @brief Removes all entries from the hash table and destroys context.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
int h_table_destroy (h_table_t * p_h_table, void (*p_free_function)(void *));

#endif //H_TABLE_LIB

//End of h_table.h file
