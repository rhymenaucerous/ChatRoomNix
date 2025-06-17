#ifndef QUEUE_LIB
#define QUEUE_LIB

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

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
 * @brief Doubly linked list node.
 * 
 * @param p_data pointer to user-supplied data for queue node.
 * @param p_next pointer to next node.
 * @param p_prev pointer to previous node.
 * 
 */
typedef struct queue_node_t {
    void * p_data;
    struct queue_node_t * p_next;
    struct queue_node_t * p_prev;
} queue_node_t;

/**
 * @brief Context structure for the queue. Points to head, tail and contains
 * size.
 * 
 * @param p_head pointer to the head of the queue.
 * @param p_tail pointer to the tail of the queue.
 * @param size of the queue.
 */
typedef struct queue_t {
    struct queue_node_t * p_head;
    struct queue_node_t * p_tail;
    int size;
} queue_t;

/**
 * @brief Initializes queue context.
 * 
 * @return queue_t* pointer to queue context. NULL returned if calloc fails.
 */
queue_t * queue_init();

/**
 * @brief Returns boolean true or false.
 * 
 * @param p_queue pointer to queue structure.
 * @return true if queue is empty.
 * @return false is queue isn't empty.
 */
bool queue_isempty (queue_t * p_queue);

/**
 * @brief Abstracts the return of queue size.
 * 
 * @param p_queue pointer to queue structure.
 * @return int returns queue size. If p_queue is NULL, -1 returned as 1 is
 * legitmate size.
 */
int queue_size (queue_t * p_queue);

/**
 * @brief Returns item at the top of the queue.
 * 
 * @param p_queue pointer to queue structure.
 * @return void* pointer to data at top of queue. Will return NULL if 
 * function fails.
 */
void * queue_peek (queue_t * p_queue);

/**
 * @brief Prints all data in the queue to the terminal. Used for testing.
 * 
 * @param p_queue pointer to queue structure.
 */
void queue_print (queue_t * p_queue);

/**
 * @brief Enqueues a node into the queue. It will become the first node in 
 * the list.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_data pointer to user-supplied data.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int queue_enqueue (queue_t * p_queue, void * data);

/**
 * @brief Dequeues a node from the queue.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_free_function user-supplied free function.
 * @return void* returns void pointer to data. Returns NULL if function fails
 * or free function input utilized.
 */
void * queue_dequeue (queue_t * p_queue, void (*p_free_function)(void *));

/**
 * @brief Removes all nodes and destroys queue context.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int queue_full_destroy (queue_t ** pp_queue, void (*p_free_function)(void *));

#endif //QUEUE_LIB
