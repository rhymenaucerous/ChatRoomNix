#ifndef CLL_LIB
#define CLL_LIB

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

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

/**
 * @brief cll node strucutre.
 * 
 * @param p_data pointer to the user-supplied data in the node.
 * @param p_next pointer to the next node.
 */
typedef struct node_t {
    void * p_data;
    struct node_t * p_next;
} node_t;

/**
 * @brief cll_t context structure.
 * 
 * @param p_head pointer to the head of the cll.
 * @param p_tail pointer to the tail of the cll.
 * @param size number of nodes in the cll.
 */
typedef struct cll_t {
    struct node_t * p_head;
    struct node_t * p_tail;
    int size;
} cll_t;

/**
 * @brief Creates circularly linked list context.
 * 
 * @return cll_t * Pointer to circularly linked list context. Returns NULL
 * if initialization fails.
 */
cll_t *
cll_init ();

/**
 * @brief Astracts size return for cll_t structure.
 * 
 * @param p_cll pointer to cll context.
 * @return int size of cll returned. If failure, -1 returned as 1 is valid 
 * size value.
 */
int
cll_size (cll_t * p_cll);

/**
 * @brief Finds the first occurance of a non-NULL item in a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @return int occurence position returned. If failure, -1 returned as 1 is  
 * valid size value.
 */
int
cll_find_occurence (cll_t * p_cll);

/**
 * @brief Used for testing purposes. Assumes all elements can be type cast
 * to int and prints to terminal.
 * 
 * @param p_cll pointer to cll context.
 */
void
cll_print (cll_t * p_cll);

/**
 * @brief Inserts an element at the beginning of the cll.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element_begin (cll_t * p_cll, void * data);

/**
 * @brief inserts an element at the end of a cll.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element_end (cll_t * p_cll, void * data);

/**
 * @brief Inserts an element at the specified position of a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @param position user-specified position in the linked list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element (cll_t * p_cll, void * data, int position);

/**
 * @brief Removes a node from the beginning of the cll.
 * 
 * @param p_cll pointer to cll context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element_begin (cll_t * p_cll, void (*p_free_function)(void *));

/**
 * @brief Removes an element from the end of a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element_end (cll_t * p_cll, void (*p_free_function)(void *));

/**
 * @brief Removes an element from the cll at a specified position.
 * 
 * @param p_cll pointer to cll context.
 * @param position user-specified position.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element (cll_t * p_cll, int position, void (*p_free_function)(void *));

/**
 * @brief Destroys all elements of a linked list and the linked list context.
 * 
 * @param pp_cll double pointer to the cll.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_destroy(cll_t ** pp_cll, void (*p_free_function)(void *));

/**
 * @brief Returns an element
 * 
 * @param p_cll pointer to cll context.
 * @param position user-specified position.
 * @return void* pointer to data returned. If error occurs, NULL will be
 * returned.
 */
void *
cll_return_element (cll_t * p_cll, int position);

/**
 * @brief Returns the last element in a list and removes it from the list.
 * 
 * @param p_cll pointer to the circularly linked list.
 * @return void* returns a pointer to the data from the node. Will return
 * NULL upon failure.
 */
void *
cll_return_element_end (cll_t * p_cll);

/**
 * @brief cll_sort sorts a linked list alphanumerically. This function will
 * type cast all data to char in order to compare. Will return NULL if error
 * occurs.
 * 
 * @param p_cll pointer to cll context.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_sort (cll_t * p_cll);

#endif //CLL_LIB

//End of cll.h library source file.
