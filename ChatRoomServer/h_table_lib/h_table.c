#include "h_table.h"

/**
 * @brief Default hash function for library. Used if user does not supply
 * function. Implemnets FNV-1 hash.
 * Pseudo code pulled from:
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * 
 * @param p_key pointer to user-supplied key.
 * @return int returns hash or FAILURE_NEGATIVE (-1).
 */
static int64_t
h_table_hash_function (const void * p_key)
{
    if (NULL == p_key)
    {
        fprintf(stderr, "h_table_hash_function: p_key NULL\n");
        return FAILURE_NEGATIVE;
    }

    //The output will be a signed 4 byte int (for error handling), so the
    //calculations can be done with a signed 2 byte int. 
    const uint32_t FNV_offset = 2166136261;
    const uint32_t FNV_prime = 16777619;
    uint32_t hash = FNV_offset;

    char * data = (char *)p_key;

    for (uint8_t counter = 0; counter < KEY_LENGTH; counter++)
    {
        hash = hash * FNV_prime;
        hash ^= data[counter];
    }

    return hash;
}

/**
 * @brief Initializes hash table context. Sets hash function and initial size
 * and capacity.
 * 
 * @param capacity The user-supplied initial capacity of the hash table.
 * @param p_hash_function user-supplied function for hashing the keys. accepts
 * a const void and returns an int.
 * @return h_table_t* pointer to hash table context structure.
 */
h_table_t *
h_table_init (uint16_t capacity, int64_t (*p_hash_function) (const void *))
{
    h_table_t * p_h_table = calloc(1, sizeof(h_table_t));

    if (NULL == p_h_table)
    {
        perror("h_table_init: p_h_table calloc\n");
        return NULL;
    }

    if (NULL != p_hash_function)
    {
        p_h_table->p_hash_function = p_hash_function;
    }
    else
    {
        int64_t (*p_default_hash_function) (const void *);
        p_default_hash_function = &h_table_hash_function;
        p_h_table->p_hash_function = p_default_hash_function;
    }

    p_h_table->capacity = capacity;

    p_h_table->pp_array = calloc(capacity, sizeof(cll_t *));

    if (NULL == p_h_table->pp_array)
    {
        perror("h_table_init: p_h_table->pp_array calloc\n");
        FREE(p_h_table);
        return NULL;
    }

    return p_h_table;
}

/**
 * @brief After the hash has been calculated, function checks if the intialized
 * list at the array index (no check done if array index empty) already has the
 * specified key.
 * 
 * @param p_cll pointer to the initialized list at the specified array index.
 * @param p_key pointer to the key of the data to be entered into the hash
 * table.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
static int
h_table_duplicate_data_check (cll_t * p_cll, const void * p_key)
{
    if ((NULL == p_cll) || (NULL == p_key))
    {
        fprintf(stderr, "h_table_duplicate_data_check: input NULL\n");
        return FAILURE;
    }

    for (int counter = 0; counter < cll_size(p_cll); counter++)
    {
        entry_t * p_temp_entry = cll_return_element(p_cll, counter);
        if (NULL == p_temp_entry)
        {
            fprintf(stderr, "h_table_duplicate_data_check: cll_return_element "
                                                                 "failure\n");
            return FAILURE;
        }

        if (SUCCESS == strncmp((const char *)p_key, 
                            (const char *)(p_temp_entry->p_key), KEY_LENGTH))
        {
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * @brief Helper function that adds a new entry to the hash table.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_new_entry pointer to the entry structure of a new entry.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
static int
h_table_new_entry_add (h_table_t * p_h_table, entry_t * p_new_entry)
{
    if ((NULL == p_h_table) || (NULL == p_h_table->p_hash_function) ||
                                               (NULL == p_new_entry))
    {
        fprintf(stderr, "h_table_new_entry_add: input NULL\n");
        return FAILURE;
    }
    
    int64_t hash = p_h_table->p_hash_function(p_new_entry->p_key);

    if (FAILURE_NEGATIVE == hash)
    {
        fprintf(stderr, "h_table_new_entry: p_hash_function failure\n");
        return FAILURE;
    }

    int64_t entry_index = hash % (int64_t)(p_h_table->capacity);
    
    if (NULL == p_h_table->pp_array[entry_index])
    {
        cll_t * p_temp_cll = cll_init();

        if (NULL == p_temp_cll)
        {
            fprintf(stderr, "h_table_new_entry: cll_init failure\n");
            return FAILURE;
        }

        if (FAILURE == cll_insert_element_end(p_temp_cll, p_new_entry))
        {
            fprintf(stderr, "h_table_new_entry: cll_insert_element_end "
                                                          "failure\n");
            return FAILURE;
        }

        p_h_table->pp_array[entry_index] = p_temp_cll;
    }
    else
    {
        cll_t * p_temp_cll = p_h_table->pp_array[entry_index];

        if (FAILURE == h_table_duplicate_data_check(p_temp_cll, 
                                           p_new_entry->p_key))
        {
            fprintf(stderr, "h_table_new_entry: duplicate data\n");
            return FAILURE;
        }

        if (FAILURE == cll_insert_element_end(p_temp_cll, p_new_entry))
        {
            fprintf(stderr, "h_table_new_entry: cll_insert_element_end "
                                                          "failure\n");
            return FAILURE;
        }
    }

    p_h_table->size++;

    return SUCCESS;
}

/**
 * @brief Calculates the load factor of the hash table.
 * Ensures 0 <= load_factor <= 1.
 * 
 * @param p_h_table pointer to the hash table context.
 * @return double load factor value or FAILURE_NEGATIVE (-1).
 */
static double
h_table_load_factor (h_table_t * p_h_table)
{
    if (0.0 == ((double) p_h_table->capacity))
    {
        fprintf(stderr, "h_table_load_factor: h_table capacity = 0\n");
        return FAILURE_NEGATIVE;
    }

    double load_factor = ((double) p_h_table->size)/
                     ((double) p_h_table->capacity);

    if ((load_factor < 0) || (load_factor > 1))
    {
        fprintf(stderr, "h_table_load_factor: load factor is out of bounds");
        return FAILURE_NEGATIVE;
    }

    return load_factor;
}

/**
 * @brief Helper funciton to re-hash function. Returns a list of all table
 * entries and removes all entries from the current table.
 * 
 * @param p_h_table pointer to the hash table context.
 * @return cll_t* pointer to a list of all the entries in the table.
 */
static cll_t *
h_table_list (h_table_t * p_h_table)
{
    if (NULL == p_h_table)
    {
        fprintf(stderr, "h_table_init: p_h_table NULL\n");
        return NULL;
    }

    cll_t * p_h_table_list = cll_init();

    for (int counter = 0; counter < p_h_table->capacity; counter++)
    {
        if (NULL != p_h_table->pp_array[counter])
        {
            cll_t * p_temp_cll = p_h_table->pp_array[counter];

            for (int counter2 = 0; counter2 < cll_size(p_temp_cll); counter2++)
            {
                entry_t * p_data_holder = cll_return_element(p_temp_cll,
                                                             counter2);

                if (NULL == p_data_holder)
                {
                    fprintf(stderr, "h_table_list: cll_return_element "
                                                         "failure\n");
                    return NULL;
                }
                if (FAILURE == cll_insert_element_end(p_h_table_list, 
                                                      p_data_holder))
                {
                    fprintf(stderr, "h_table_list: cll_insert_element_end "
                                                             "failure\n");
                    return NULL;
                }
            }

            if (FAILURE == cll_destroy(&p_temp_cll, NULL))
            {
                fprintf(stderr, "h_table_list: cll_destroy failure\n");
                return NULL;
            }

            p_h_table->pp_array[counter] = NULL;
        }
    }

    return p_h_table_list;
}

//puts all the list elements back into the table
/**
 * @brief Helper funciton to re-hash function. Enters all entries from the
 * h_table_list function into the re-sized hash table.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_cll pointer ot the list with all hash table entries.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
static int
h_table_list_to_table (h_table_t * p_h_table, cll_t * p_cll)
{
    if ((NULL == p_h_table) || (NULL == p_cll))
    {
        fprintf(stderr, "h_table_init: input NULL\n");
        return FAILURE;
    }

    for (int counter = 0; counter < cll_size(p_cll); counter++)
    {
        entry_t * p_temp_entry = cll_return_element(p_cll, counter);

        if (NULL == p_temp_entry)
        {
            fprintf(stderr, "h_table_list_to_table: cll_return_element "
                                                          "failure\n");
            return FAILURE;
        }

        if (FAILURE == h_table_new_entry_add(p_h_table, p_temp_entry))
        {
            fprintf(stderr, "h_table_list_to_table: h_table_new_entry_add "
                                                             "failure\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * @brief Re-hashes the table once the load factor exceeds 0.75.
 * This load factor value was taken from literature on the hash table subject.
 * Due to unknown table functionality requirements, generalized guidance 
 * taken.
 * 
 * @param p_h_table pointer to the hash table context.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
static int
h_table_re_hash (h_table_t * p_h_table)
{
    if (NULL == p_h_table)
    {
        fprintf(stderr, "h_table_init: p_h_table NULL\n");
        return FAILURE;
    }

    double load_factor = h_table_load_factor(p_h_table);

    if (FAILURE_NEGATIVE == load_factor)
    {
        fprintf(stderr, "h_table_list: h_table_load_factor failure\n");
        return FAILURE;
    }

    if (load_factor < 0.75)
    {
        return SUCCESS;
    }

    cll_t * p_temp_cll = h_table_list(p_h_table);

    if (NULL == p_temp_cll)
    {
        fprintf(stderr, "h_table_re_hash: h_table_list failure\n");
        return FAILURE;
    }

    p_h_table->size = 0;

    uint16_t capacity_holder = p_h_table->capacity;

    //A hash table's size greatly impacts how often clusters form. when the
    //size is prime, that clustering happens less often.
    p_h_table->capacity = next_prime(capacity_holder);

    if (FAILURE == p_h_table->capacity)
    {
        fprintf(stderr,"h_table_re_hash: next_prime failure\n");
        return FAILURE;
    }

    //If the hash table size goes above 65521 (the highest prime number under 
    //65535) an error will be returned. Since the capacity is uint16_t type, 
    //it will assign the bits so that the output is lower.
    if (p_h_table->capacity < capacity_holder)
    {
        fprintf(stderr, "h_table_re_hash: hash table maximum size exceeded");
        return FAILURE;
    }

    p_h_table->pp_array = realloc(p_h_table->pp_array, p_h_table->capacity * 
                                                          sizeof(cll_t *));

    if (NULL == p_h_table->pp_array)
    {
        perror("h_table_re_hash: pp_array realloc failure");
        return FAILURE;
    }

    //Reset array to NULL's following reallocation 
    for (size_t counter = 0; counter < p_h_table->capacity; counter++)
    {
        p_h_table->pp_array[counter] = NULL;
    }

    if (FAILURE == h_table_list_to_table(p_h_table, p_temp_cll))
    {
        fprintf(stderr, "h_table_re_hash: h_table_list_to_table failure\n");
        return FAILURE;
    }

    if (FAILURE == cll_destroy(&p_temp_cll, NULL))
    {
        fprintf(stderr, "h_table_list: cll_destroy failure\n");
        return FAILURE;
    }
    
    return SUCCESS;

}

/**
 * @brief Function that adds new entries to the hash table.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_data user supplied data to be entered into hash table.
 * @param p_key pointer to user-supplied key.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
int
h_table_new_entry (h_table_t * p_h_table, void * p_data, const void * p_key)
{
    if ((NULL == p_h_table) || (NULL == p_data) || (NULL == p_key))
    {
        fprintf(stderr, "h_table_new_entry: input NULL\n");
        return FAILURE;
    }

    if (FAILURE == h_table_re_hash(p_h_table))
    {
        fprintf(stderr, "h_table_new_entry: h_table_re_hash failure");
        return FAILURE;
    }
    
    entry_t * p_new_entry = calloc(1, sizeof(entry_t));

    if (NULL == p_new_entry)
    {
        perror("h_table_new_entry: p_new_entry calloc\n");
        return FAILURE;
    }

    p_new_entry->p_data = p_data;
    p_new_entry->p_key = p_key;
    p_new_entry->p_free_function = NULL;

    if (FAILURE == h_table_new_entry_add(p_h_table, p_new_entry))
    {
        fprintf(stderr, "h_table_new_entry: h_table_new_entry_add failure\n");
        FREE(p_new_entry);
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Returns an entry from the hash table given a key.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_key pointer to user-supplied key.
 * @return void* pointer to specified data. Will return NULL if function fails.
 */
void *
h_table_return_entry (h_table_t * p_h_table, void * p_key)
{
    if ((NULL == p_h_table) ||(NULL == p_key))
    {
        fprintf(stderr, "h_table_return_entry: input NULL\n");
        return NULL;
    }


    if (NULL == p_h_table->p_hash_function)
    {
        fprintf(stderr, "h_table_return_entry: hash function NULL\n");
        return NULL;
    }
    
    int64_t hash = p_h_table->p_hash_function(p_key);

    if (FAILURE_NEGATIVE == hash)
    {
        fprintf(stderr, "h_table_return_entry: p_hash_function failure\n");
        return NULL;
    }

    int64_t entry_index = hash % (int64_t)(p_h_table->capacity);
    
    if (NULL == p_h_table->pp_array[entry_index])
    {
        return NULL;
    }
    else
    {
        cll_t * p_temp_cll = p_h_table->pp_array[entry_index];

        for (int counter = 0; counter < cll_size(p_temp_cll); counter++)
        {
            entry_t * p_temp_entry = cll_return_element(p_temp_cll, counter);
            
            if (SUCCESS == strncmp((const char *)p_key, 
                                (const char *)(p_temp_entry->p_key),
                                KEY_LENGTH))
            {
                return p_temp_entry->p_data;
            }
        }
    }

    return NULL;
}

static void
h_table_free_helper (void * p_data)
{
    entry_t * p_temp_entry = (entry_t *) p_data;

    if (NULL == p_temp_entry->p_free_function)
    {
        FREE(p_temp_entry);
    }
    else
    {
        p_temp_entry->p_free_function(p_temp_entry->p_data);
        FREE(p_temp_entry);
    }
}

/**
 * @brief Destroys an entry in the hash table given a key.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_key pointer to user-supplied key.
 * @return void* pointer to specified data. Returns NULL if function fails.
 */
void *
h_table_destroy_entry (h_table_t * p_h_table, void * p_key)
{
    if ((NULL == p_h_table) ||(NULL == p_key))
    {
        fprintf(stderr, "h_table_destroy_entry: input NULL\n");
        return NULL;
    }


    if (NULL == p_h_table->p_hash_function)
    {
        fprintf(stderr, "h_table_destroy_entry: hash function NULL\n");
        return NULL;
    }
    
    int64_t hash = p_h_table->p_hash_function(p_key);

    if (FAILURE_NEGATIVE == hash)
    {
        fprintf(stderr, "h_table_destroy_entry: p_hash_function failure\n");
        return NULL;
    }

    int64_t entry_index = hash % (int64_t)(p_h_table->capacity);
    
    if (NULL == p_h_table->pp_array[entry_index])
    {
        fprintf(stderr, "h_table_destroy_entry: entry not found, key "
                                                        "invalid\n");
        return NULL;
    }
    else
    {
        cll_t * p_temp_cll = p_h_table->pp_array[entry_index];

        for (int counter = 0; counter < cll_size(p_temp_cll); counter++)
        {
            entry_t * p_temp_entry = cll_return_element(p_temp_cll, counter);
            
            if (SUCCESS == strncmp((const char *)p_key, 
                            (const char *)(p_temp_entry->p_key), KEY_LENGTH))
            {                
                if (FAILURE == cll_remove_element(p_temp_cll, counter, NULL))
                {
                    fprintf(stderr, "h_table_destroy_entry: cll_remove_element"
                                                                 " failure\n");
                    return NULL;
                }

                if (0 == cll_size(p_temp_cll))
                {
                    if (FAILURE == cll_destroy(&p_temp_cll, NULL))
                    {
                        fprintf(stderr, "h_table_destroy_entry: cll_destroy "
                                                               "failure\n");
                        return NULL;
                    }

                    p_h_table->pp_array[entry_index] = NULL;
                }

                void * p_data_holder = p_temp_entry->p_data;

                FREE(p_temp_entry);

                p_h_table->size--;

                return p_data_holder;
            }
        }
    }

    return NULL;
}

/**
 * @brief Removes all entries from the hash table and destroys context.
 * 
 * @param p_h_table pointer to the hash table context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1 respectively) returned.
 */
int
h_table_destroy (h_table_t * p_h_table, void (*p_free_function)(void *))
{
    if (NULL == p_h_table)
    {
        fprintf(stderr, "h_table_destroy: h_table NULL\n");
        return FAILURE;
    }

    if (NULL == p_h_table->pp_array)
    {
        fprintf(stderr, "h_table_destroy: pp_array NULL\n");
        return FAILURE;
    }

    for (int counter = 0; counter < p_h_table->capacity; counter++)
    {
        if (NULL != p_h_table->pp_array[counter])
        {           
            for (int counter2 = 0; counter2 < 
                 cll_size(p_h_table->pp_array[counter]); counter2++)
            {
                entry_t * p_temp_entry = cll_return_element(
                                        p_h_table->pp_array[counter], counter2);

                p_temp_entry->p_free_function = p_free_function;
            }
            
            if (FAILURE == cll_destroy(&(p_h_table->pp_array[counter]), 
                                                 &h_table_free_helper))
            {
                fprintf(stderr, "h_table_destroy: cll_destroy failure");
                return FAILURE;
            }
        }
    }

    FREE(p_h_table->pp_array);
    FREE(p_h_table);

    return SUCCESS;
}

//End of h_table.c file
