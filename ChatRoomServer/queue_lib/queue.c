#include "queue.h"

/**
 * @brief Initializes queue context.
 * 
 * @return queue_t* pointer to queue context. NULL returned if calloc fails.
 */
queue_t *
queue_init()
{
    queue_t * p_queue = calloc(1, sizeof(queue_t));

    if (NULL == p_queue)
    {
        perror("queue_init: p_queue calloc");
        return NULL;
    }
    
    return p_queue;
}

/**
 * @brief Returns boolean true or false.
 * 
 * @param p_queue pointer to queue structure.
 * @return true if queue is empty.
 * @return false is queue isn't empty.
 */
bool
queue_isempty (queue_t * p_queue)
{
    return (0 == p_queue->size);
}

/**
 * @brief Abstracts the return of queue size.
 * 
 * @param p_queue pointer to queue structure.
 * @return int returns queue size. If p_queue is NULL, -1 returned as 1 is
 * a legitmate size.
 */
int
queue_size (queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_size: p_queue NULL\n");
        return FAILURE_NEGATIVE;
    }
    return p_queue->size;
}

/**
 * @brief Returns item at the top of the queue.
 * 
 * @param p_queue pointer to queue structure.
 * @return void* pointer to data at top of queue. Will return NULL if 
 * function fails.
 */
void *
queue_peek (queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_peek: p_queue NULL\n");
        return NULL;
    }

    if (0 == p_queue->size)
    {
        fprintf(stderr, "queue_peek: queue is empty!\n");
        return NULL;
    }

    if (NULL == p_queue->p_tail)
    {
        fprintf(stderr, "queue_peek: p_queue tail NULL\n");
        return NULL;
    }

    return p_queue->p_tail->p_data;
}

/**
 * @brief Prints all data in the queue to the terminal. Used for testing.
 * 
 * @param p_queue pointer to queue structure.
 */
void
queue_print (queue_t * p_queue)
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_print: p_queue NULL\n");
        return;
    }

    printf("\nPrinting queue data:\n");

    queue_node_t * p_temp_node = p_queue->p_head;

    for (int counter = 0; counter < p_queue->size; counter++)
    {
        if (NULL == p_temp_node)
        {
            fprintf(stderr, "queue_print: p_queue node NULL");
            return;
        }
        printf("Data in node %d:%d\n", counter, *(int *) (p_temp_node->p_data));
        p_temp_node = p_temp_node->p_next;
    }
}

/**
 * @brief Enqueues a node into the queue. It will become the first node in 
 * the list.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_data pointer to user-supplied data.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
queue_enqueue (queue_t * p_queue, void * p_data)
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_enqueue: p_queue NULL\n");
        return FAILURE;
    }

    queue_node_t * p_queue_node = calloc(1, sizeof(queue_node_t));

    if (NULL == p_queue_node)
    {
        perror("queue_enqueue: p_queue_node calloc");
        return FAILURE;
    }

    p_queue_node->p_data = p_data;

    if (0 == p_queue->size)
    {
        p_queue->p_head = p_queue_node;
        p_queue->p_tail = p_queue_node;
        p_queue->size = 1;

        p_queue_node->p_next = NULL;
        p_queue_node->p_prev = NULL;

        return SUCCESS;
    }

    if (NULL == p_queue->p_head)
    {
        fprintf(stderr, "queue_enqueue: p_queue head NULL\n");
        FREE(p_queue_node);
        return FAILURE;
    }

    p_queue->p_head->p_prev = p_queue_node;
    p_queue_node->p_next = p_queue->p_head;
    p_queue->p_head = p_queue_node;
    p_queue_node->p_prev = NULL;
    p_queue->size++;

    return SUCCESS;
}

/**
 * @brief Frees data with nodes within the queue.
 * 
 * @param p_queue_node pointer to the queue node specified.
 * @param p_free_function user-supplied free function.
 */
static void
queue_p_data_free (queue_node_t * p_queue_node, void (*p_free_function)(void *))
{
    if ((NULL == p_free_function) || (NULL == p_queue_node->p_data))
    {
        return;
    }

    p_free_function(p_queue_node->p_data);
    p_queue_node->p_data = NULL;
}

/**
 * @brief Dequeues a node from the queue.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_free_function user-supplied free function.
 * @return void* returns void pointer to data. Returns NULL if function fails.
 */
void *
queue_dequeue (queue_t * p_queue, void (*p_free_function)(void *))
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_dequeue: p_queue NULL\n");
        return NULL;
    }

    if (0 == p_queue->size)
    {
        fprintf(stderr, "queue_dequeue: The queue is already empty!\n");
        return NULL;
    }

    if (NULL == p_queue->p_tail)
    {
        fprintf(stderr, "queue_dequeue: p_queue tail NULL\n");
        return NULL;
    }

    queue_node_t * p_temp_node = p_queue->p_tail;

    void * data_holder = p_temp_node->p_data;

    if (1 == p_queue->size)
    {
        queue_p_data_free(p_temp_node, p_free_function);
        FREE(p_temp_node);
        p_queue->p_head = NULL;
        p_queue->p_tail = NULL;
        p_queue->size = 0;

        if (NULL == p_free_function)
        {
            return data_holder;
        }
        else
        {
            return NULL;
        }
    }

    p_queue->p_tail = p_queue->p_tail->p_prev;
    p_queue->p_tail->p_next = NULL;
    p_queue->size--;

    queue_p_data_free(p_temp_node, p_free_function);
    FREE(p_temp_node);

    if (NULL == p_free_function)
    {
        return data_holder;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Dequeues a node from the queue.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_free_function user-supplied free function.
 * @return void* returns void pointer to data. Returns NULL if function fails.
 */
static int
queue_destroy_dequeue (queue_t * p_queue, void (*p_free_function)(void *))
{
    if (NULL == p_queue)
    {
        fprintf(stderr, "queue_dequeue: p_queue NULL\n");
        return FAILURE;
    }

    if (0 == p_queue->size)
    {
        fprintf(stderr, "queue_dequeue: The queue is already empty!\n");
        return FAILURE;
    }

    if (NULL == p_queue->p_tail)
    {
        fprintf(stderr, "queue_dequeue: p_queue tail NULL\n");
        return FAILURE;
    }

    queue_node_t * p_temp_node = p_queue->p_tail;

    if (1 == p_queue->size)
    {
        queue_p_data_free(p_temp_node, p_free_function);
        FREE(p_temp_node);
        p_queue->p_head = NULL;
        p_queue->p_tail = NULL;
        p_queue->size = 0;

        return SUCCESS;
    }

    p_queue->p_tail = p_queue->p_tail->p_prev;
    p_queue->p_tail->p_next = NULL;
    p_queue->size--;

    queue_p_data_free(p_temp_node, p_free_function);
    FREE(p_temp_node);

    return SUCCESS;
}

/**
 * @brief Destroys queue context.
 * 
 * @param p_queue pointer to queue structure.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
static int
queue_context_destroy (queue_t ** pp_queue)
{
    if (NULL == *pp_queue)
    {
        fprintf(stderr, "queue_context_destroy: *p_queue Memory Error\n");
        return FAILURE;
    }

    FREE(*pp_queue);

    return SUCCESS;
}

/**
 * @brief Removes all nodes and destroys queue context.
 * 
 * @param p_queue pointer to queue structure.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
queue_full_destroy (queue_t ** pp_queue, void (*p_free_function)(void *))
{
    if (NULL == *pp_queue)
    {
        fprintf(stderr, "queue_full_destroy: *pp_queue Memory Error\n");
        return FAILURE;
    }

    while ((*pp_queue)->size != 0)
    {
        if (FAILURE == queue_destroy_dequeue(*pp_queue, p_free_function))
        {
            fprintf(stderr, "queue_full_destroy: queue_dequeue failure\n");
            return FAILURE;
        }
    }

    return queue_context_destroy(pp_queue);
}