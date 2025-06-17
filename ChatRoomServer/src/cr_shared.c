#include "../include/cr_shared.h"

/**
 * @brief Simple function to check if an int variable is a valid port number.
 * 
 * @param p_port int pointer to port number.
 * @return int SUCCESS or FAILURE (0 or 1 respectively).
 */
int
port_range_check (long * p_port)
{
    if (NULL == p_port)
    {
        fprintf(stderr, "set_config_members: input NULL.\n");
        return FAILURE;
    }
    
    if ((0 == *p_port) || (65535 < *p_port))
    {
        fprintf(stderr, "port_range_check: config port out of range.\n");
        return FAILURE;
    }
    
    return SUCCESS;
}

/**
 * @brief when supplied to the h_table_destroy function, the following function
 * will be used to free the memory of each entry in the hash table. 
 * 
 * @param p_room_entry_holder The input into this function is a pointer to an
 * hash table entry that is type room_t (the pointer is void though).
 */
void
free_rooms (void * p_room_entry_holder)
{
    if (NULL == p_room_entry_holder)
    {
        fprintf(stderr, "free_rooms: input NULL\n");
        return;
    }
    
    room_t * p_room_entry = p_room_entry_holder;

    if (FAILURE == cll_destroy(&p_room_entry->p_users, NULL))
    {
        fprintf(stderr, "free_rooms: cll_destroy()\n");
    }

    if (SUCCESS != pthread_mutex_destroy(&p_room_entry->room_mutex))
    {
        perror("free_rooms: pthread_mutex_destroy:");
    }

    if (SUCCESS != remove(p_room_entry->p_room_location))
    {
        perror("free_rooms: remove:");
    }

    FREE(p_room_entry);
}

//End of cr_shared.c file
