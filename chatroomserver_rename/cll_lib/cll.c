#include "cll.h"

/**
 * @brief Creates circularly linked list context.
 * 
 * @return cll_t * Pointer to circularly linked list context. Returns NULL
 * if initialization fails.
 */
cll_t *
cll_init ()
{
    cll_t * p_cll = (calloc(1, sizeof(cll_t)));

    if (NULL == p_cll)
    {
        perror("cll_init: p_cll calloc");
        return NULL;
    }
    
    return p_cll;
}

/**
 * @brief Astracts size return for cll_t structure.
 * 
 * @param p_cll pointer to cll context.
 * @return int size of cll returned. If failure, -1 returned as 1 is valid 
 * size value.
 */
int
cll_size (cll_t * p_cll)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_size: p_cll not allocated\n");
        return FAILURE_NEGATIVE;
    }

    return p_cll->size;
}

/**
 * @brief Finds the first occurance of a non-NULL item in a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @return int occurence position returned. If failure, -1 returned as 1 is  
 * valid size value.
 */
int
cll_find_occurence (cll_t * p_cll)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_find_occurence: p_cll NULL\n");
        return FAILURE_NEGATIVE;
    }

    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cll_find_occurence: p_cll head NULL\n");
        return FAILURE_NEGATIVE;
    }

    int counter = 1;
    node_t * p_temp = p_cll->p_head;

    while (counter <= p_cll->size)
    {
        if (NULL != p_temp->p_data)
        {
            return counter;
        }

        counter++;

        if (NULL == p_temp->p_next)
        {
            fprintf(stderr, "cll_find_occurence: p_cll node NULL\n");
            return FAILURE_NEGATIVE;
        }

        p_temp = p_temp->p_next;
    }

    return FAILURE_NEGATIVE;
}

/**
 * @brief Used for testing purposes. Assumes all elements can be type cast
 * to int and prints to terminal.
 * 
 * @param p_cll pointer to cll context.
 */
void
cll_print (cll_t * p_cll)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_insert_element: p_cll not allocated\n");
        return;
    }

    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cll_print: p_cll head NULL\n");
        return;
    }

    int counter = 0;
    node_t *p_temp = p_cll->p_head;

    printf("Size of linked list:%d\n", p_cll->size);

    while (counter < p_cll->size)
    {
        printf("Data in node %d:%ld\n", counter, *(uint64_t *) (p_temp->p_data));
        counter++;

        if (NULL == p_temp->p_next)
        {
            fprintf(stderr, "cll_print: p_cll node NULL\n");
            return;
        }

        p_temp = p_temp->p_next;
    }
}

/**
 * @brief Handles the case of an empty list for inser functions.
 * 
 * @param p_cll pointer to cll context.
 * @param p_newnode node being added.
 * @return int success or failure indicator.
 */
static int
cll_empty_list_add (cll_t * p_cll, node_t * p_newnode)
{
    if (NULL == p_cll->p_head)
    {
        p_cll->p_head = p_newnode;
        p_cll->p_tail = p_newnode;
        p_cll->size = 1;
        p_newnode->p_next = p_newnode;

        return SUCCESS;
    }

    return FAILURE;
}

/**
 * @brief Inserts an element at the beginning of the cll.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element_begin (cll_t * p_cll, void * data)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_insert_element_begin: p_cll NULL\n");
        return FAILURE;
    }

    node_t * p_newnode = (calloc(1, sizeof(node_t)));

    if (NULL == p_newnode)
    {
        perror("cll_insert_element_begin: node_t calloc");
        return FAILURE;
    }

    p_newnode->p_data = data;

    int checker = cll_empty_list_add(p_cll, p_newnode);

    if (SUCCESS == checker)
    {
        return checker;
    }

    if (NULL == p_cll->p_tail)
    {
        fprintf(stderr, "cll_insert_element_begin: p_cll tail NULL\n");
        FREE(p_newnode);
        return FAILURE;
    }

    p_newnode->p_next = p_cll->p_head;
    p_cll->p_head = p_newnode;
    p_cll->p_tail->p_next = p_newnode;
    p_cll->size++;

    return SUCCESS;
}

/**
 * @brief inserts an element at the end of a cll.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element_end (cll_t * p_cll, void * data)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_insert_element_end: p_cll NULL\n");
        return FAILURE;
    }

    node_t * p_newnode = (calloc(1, sizeof(node_t)));

    if (NULL == p_newnode)
    {
        perror("cll_insert_element_begin: node_t calloc");
        return FAILURE;
    }

    p_newnode->p_data = data;

    int checker = cll_empty_list_add(p_cll, p_newnode);

    if (SUCCESS == checker)
    {
        return SUCCESS;
    }

    if (NULL == p_cll->p_tail)
    {
        fprintf(stderr, "cll_insert_element_end: p_cll tail NULL\n");
        FREE(p_newnode);
        return FAILURE;
    }

    //All cases other than an empty list can be handled with the same
    //algorithm.
    p_cll->p_tail->p_next = p_newnode;
    p_cll->p_tail = p_newnode;
    p_newnode->p_next = p_cll->p_head;
    p_cll->size++;

    return SUCCESS;
}

/**
 * @brief Returns an int, either 0 or 1, determining whether the position
 * is in bounds or not.
 * 
 * @param p_cll pointer to cll context.
 * @param position user-specified position in the linked list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
static int
cll_position_checker (cll_t * p_cll, int position)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_insert_element: p_cll NULL\n");
        return FAILURE;
    }

    if ((position < 0) || (position > p_cll->size))
    {
        printf("\ncll_insert_element:\n"
        "position:%d\nsize:%d\n", position, (p_cll->size));
        printf("Position invalid, element not inserted. Position must be "
        "between 0 and list size, unless list size = 0.\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Inserts an element at the specified position of a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @param data user-supplied data pointer to be input into a node in the list.
 * @param position user-specified position in the linked list.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_insert_element (cll_t * p_cll, void * data, int position)
{
    int checker = SUCCESS;

    if (0 != p_cll->size)
    {
        checker = cll_position_checker(p_cll, position);

        if (FAILURE == checker)
        {
            return checker;
        }
        else if (0 == position)
        {
            checker = cll_insert_element_begin(p_cll, data);
            return checker;
        }
        else if (p_cll->size == position)
        {
            checker = cll_insert_element_end(p_cll, data);
            return checker;
        }
    }

    node_t * p_newnode = (node_t *)(calloc(1, sizeof(node_t)));

    if (NULL == p_newnode)
    {
        perror("cll_insert_element: node_t calloc");
        return FAILURE;
    }

    p_newnode->p_data = data;
    
    checker = cll_empty_list_add(p_cll, p_newnode);

    if (SUCCESS == checker)
    {
        return checker;
    }
    
    int counter = 1;
    node_t * p_temp = p_cll->p_head;
    
    while (counter < position)
    {
        counter++;

        if (NULL == p_temp)
        {
            fprintf(stderr, "cll_insert_element: node NULL");
            FREE(p_newnode);
            return FAILURE;
        }

        p_temp = p_temp->p_next;
    }
    
    p_newnode->p_next = p_temp->p_next;
    p_temp->p_next = p_newnode;
    p_cll->size++;

    return SUCCESS;
}

/**
 * @brief frees user-supplied data pointers with user-supplied free function
 * in cll nodes.
 * 
 * @param p_node pointer to a node in the cll.
 * @param p_free_function user-supplied function for freeing supplied data 
 * pointers.
 */
static void
cll_p_data_free (node_t * p_node, void (*p_free_function)(void *))
{
    if ((NULL == p_free_function) || (NULL == p_node->p_data))
    {
        return;
    }

    p_free_function(p_node->p_data);
    p_node->p_data = NULL;
}

/**
 * @brief Removes a node from the beginning of the cll.
 * 
 * @param p_cll pointer to cll context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element_begin (cll_t * p_cll, void (*p_free_function)(void *))
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_remove_element_begin: p_cll NULL\n");
        return FAILURE;
    }

    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cll_remove_element_begin: p_cll head NULL");
        return FAILURE;
    }

    if (NULL == p_cll->p_tail)
    {
        fprintf(stderr, "cll_remove_element_begin: p_cll tail NULL");
        return FAILURE;
    }
    
    //case that there is only one node in the linked list handled.
    if (1 == p_cll->size)
    {
        cll_p_data_free(p_cll->p_head, p_free_function);
        FREE(p_cll->p_head);
        p_cll->p_tail = NULL;
        p_cll->size = 0;

        return SUCCESS;
    }

    node_t * p_temp1 = p_cll->p_head;

    if (NULL == p_temp1->p_next)
    {
        fprintf(stderr, "cll_remove_element_begin: p_cll node NULL");
        return FAILURE;
    }

    p_cll->p_head = p_temp1->p_next;
    cll_p_data_free(p_temp1, p_free_function);
    FREE(p_temp1);
    p_cll->p_tail->p_next = p_cll->p_head;
    p_cll->size--;

    return SUCCESS;
}

/**
 * @brief Removes an element from the end of a linked list.
 * 
 * @param p_cll pointer to cll context.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element_end (cll_t * p_cll, void (*p_free_function)(void *))
{
    return cll_remove_element(p_cll, p_cll->size-1, p_free_function);
}

/**
 * @brief Removes an element from the cll at a specified position.
 * 
 * @param p_cll pointer to cll context.
 * @param position user-specified position.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_remove_element (cll_t * p_cll, int position,
                void (*p_free_function)(void *))
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_remove_element: p_cll NULL\n");
        return FAILURE;
    }

    //NOTE: The position checker allows position to be equal to 
    //list size for insertion purposes. for returning elements,
    //position can only be up to list size - 1. For exampe, if the list
    //is size 1, the only valid index is 0. 
    int position_holder;

    if (0 == position)
    {
        position_holder = 0;
    }
    else
    {
        position_holder = position - 1;
    }

    if (0 != p_cll->size)
    {
        int checker = cll_position_checker(p_cll, position_holder);

        if (FAILURE == checker)
        {
            return checker;
        }
        else if (0 == position)
        {
            checker = cll_remove_element_begin(p_cll, p_free_function);
            return checker;
        }
    }
    
    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cll_remove_element: p_cll head NULL\n");
        return FAILURE;
    }
    
    if (1 == p_cll->size)
    {
        cll_p_data_free(p_cll->p_head, p_free_function);
        FREE(p_cll->p_head);
        p_cll->p_tail = NULL;
        p_cll->size = 0;
        return SUCCESS;
    }

    int counter = 0;
    node_t * p_temp1 = p_cll->p_head;

    if (NULL == p_temp1->p_next)
    {
        fprintf(stderr, "cll_remove_element: p_cll node NULL\n");
        return FAILURE;
    }

    node_t * p_temp2 = p_temp1->p_next;

    while (counter < (position - 1))
    {
        counter++;
        p_temp1 = p_temp2;
    
        if (NULL == p_temp2->p_next)
        {
            fprintf(stderr, "cll_remove_element: p_cll node NULL\n");
            return FAILURE;
        }

        p_temp2 = p_temp2->p_next;
    }
    p_temp1->p_next = p_temp2->p_next;

    if (position == (p_cll->size - 1))
    {
        p_cll->p_tail = p_temp1;
    }

    cll_p_data_free(p_temp2, p_free_function);
    FREE(p_temp2);
    p_cll->size--;

    return SUCCESS;
}

/**
 * @brief Destroys all elements of a linked list and the linked list context.
 * 
 * @param pp_cll double pointer to the cll.
 * @param p_free_function user-supplied free function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int cll_destroy(cll_t ** pp_cll, void (*p_free_function)(void *))
{
    if (NULL == (*pp_cll))
    {
        fprintf(stderr, "cll_destroy: p_cll NULL\n");
        return FAILURE;
    }

    int checker = SUCCESS;

    while(0 < (*pp_cll)->size)
    {
        checker = cll_remove_element_begin((*pp_cll), p_free_function);
        if (FAILURE == checker)
        {
            return checker;
        }
    }

    FREE((*pp_cll));

    return SUCCESS;
}

/**
 * @brief Returns an element
 * 
 * @param p_cll pointer to cll context.
 * @param position user-specified position.
 * @return void* pointer to data returned. If error occurs, NULL will be
 * returned.
 */
void *
cll_return_element (cll_t * p_cll, int position)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_return_element: p_cll NULL\n");
        return NULL;
    }

    //NOTE: The position checker allows position to be equal to 
    //list size for insertion purposes. for returning elements,
    //position can only be up to list size - 1. For exampe, if the list
    //is size 1, the only valid index is 0. 
    int position_holder;

    if (0 == position)
    {
        position_holder = 0;
    }
    else
    {
        position_holder = position - 1;
    }

    if (0 != p_cll->size)
    {
        int checker = cll_position_checker(p_cll, position_holder);

        if (FAILURE == checker)
        {
            return NULL;
        }
    }
    else
    {
        fprintf(stderr, "cll_return_element: Linked list is empty");
        return NULL;
    }

    int counter = 0;
    node_t * p_temp = p_cll->p_head;

    if (NULL == p_temp)
    {
        fprintf(stderr, "cll_return_element: p_cll empty\n");
        return NULL;
    }

    while (counter != position)
    {
        if (NULL == p_temp)
        {
            fprintf(stderr, "cll_return_element: p_cll node NULL\n");
            return NULL;
        }
        p_temp = p_temp->p_next;
        counter++;
    }

    return p_temp->p_data;
}

/**
 * @brief Returns the last element in a list and removes it from the list.
 * 
 * @param p_cll pointer to the circularly linked list.
 * @return void* returns a pointer to the data from the node. Will return
 * NULL upon failure.
 */
void *
cll_return_element_end (cll_t * p_cll)
{
    if (NULL == p_cll)
    {
        fprintf(stderr, "cll_return_element_end: input NULL\n");
        return NULL;
    }

    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cll_return_element_end: list empty\n");
        return NULL;
    }

    if (1 == p_cll->size)
    {
        node_t * p_temp_node = p_cll->p_head;
        p_cll->p_head = NULL;
        p_cll->p_tail = NULL;
        p_cll->size--;

        void * p_data = p_temp_node->p_data;
        FREE(p_temp_node);
        return p_data;
    }
    else
    {
        node_t * p_temp_node = p_cll->p_head;

        while (p_temp_node->p_next != p_cll->p_tail)
        {
            if (NULL == p_temp_node->p_next)
            {
                fprintf(stderr, "cll_return_element_end: list node NULL\n");
                return NULL;
            }
            p_temp_node = p_temp_node->p_next;
        }

        //p_temp_node is now the second to last node in the list
        node_t * p_last_node = p_temp_node->p_next;
        p_cll->p_tail = p_temp_node;
        p_temp_node->p_next = p_cll->p_head;
        p_cll->size--;

        void * p_data = p_last_node->p_data;
        FREE(p_last_node);
        return p_data;
    }
}

/**
 * @brief The function will iterate through both lists and create the merged
 * list.
 * 
 * @param p_temp1 pointer node that iterates through new list while it's 
 * being created.
 * @param p_temp2 pointer node to the beginning of the new list.
 * @param p_temp3 pointer node to first half of list being merged.
 * @param p_temp4 pointer node to second half of list being merged.
 * @param size1 size of the first half of list being merged.
 * @param size2 size of the second half of list being merged.
 * @param counter1 counter for iteration through first half of list being
 * merged.
 * @param counter2 counter for iteration through second half of list being
 * merged.
 * @return node_t* returns the merged cll.
 */
static node_t * 
cll_merge_helper(node_t * p_temp1, node_t * p_temp2, node_t * p_temp3, 
                 node_t * p_temp4, int size1, int size2, 
                 int counter1, int counter2)
{
    while ((counter1 != size1) && (counter2 != size2))
    {
        if ((*(char *)(p_temp3->p_data)) < (*(char *)(p_temp4->p_data)))
        {
            if ((NULL == p_temp1) || (NULL == p_temp3))
            {
                fprintf(stderr, "cll_merge: p_node NULL\n");
                return NULL;
            }

            p_temp1->p_next = p_temp3;
            p_temp1 = p_temp1->p_next;
            p_temp3 = p_temp3->p_next;
            counter1++;
        }
        else
        {
            if ((NULL == p_temp1) || (NULL == p_temp4))
            {
                fprintf(stderr, "cll_merge: p_node NULL\n");
                return NULL;
            }

            p_temp1->p_next = p_temp4;
            p_temp1 = p_temp1->p_next;
            p_temp4 = p_temp4->p_next;
            counter2++;
        }
    }

    //Handles remaing list.
    while (counter1 != size1)
    {
        if ((NULL == p_temp1) || (NULL == p_temp3))
        {
            fprintf(stderr, "cll_merge: p_node NULL\n");
            return NULL;
        }

        p_temp1->p_next = p_temp3;
        p_temp1 = p_temp1->p_next;
        p_temp3 = p_temp3->p_next;
        counter1++;
    }
    while (counter2 != size2)
    {
        if ((NULL == p_temp1) || (NULL == p_temp4))
        {
            fprintf(stderr, "cll_merge: p_node NULL\n");
            return NULL;
        }

        p_temp1->p_next = p_temp4;
        p_temp1 = p_temp1->p_next;
        p_temp4 = p_temp4->p_next;
        counter2++;
    }

    if (NULL == p_temp1)
    {
        fprintf(stderr, "cll_merge: p_node NULL\n");
        return NULL;
    }
    


    p_temp1->p_next = p_temp2;

    return p_temp2;
}

/**
 * @brief Helper function to cll_mergesort. Merges input nodes based on 
 * alphanumeric comparison: the smallest values are placed first.
 * 
 * @param p_node1 pointer to first node list to be merged.
 * @param p_node2 pointer to second node list to be merged.
 * @param size1 the size of the first node list.
 * @param size2 the size of the second node list.
 * @return node_t* is the merged list. Returns NULL if memory access
 * error occurs.
 */
static node_t * 
cll_merge (node_t * p_node1, node_t * p_node2, int size1, int size2)
{
    
    //counter1 and counter2 will track the position in node1 and node2 
    //respectively.
    int counter1 = 0, counter2 = 0;

    //p_temp1 & 2 will be used to point at the list being created, while
    //p_temp3&4 will be used it iterated throught the lists of p_node1 and 
    //p_node2, respectively.
    node_t * p_temp1 = NULL;
    node_t * p_temp2 = NULL;
    node_t * p_temp3 = p_node1;
    node_t * p_temp4 = p_node2;

    //The first nodes of new list created.
    if ((*(char *)(p_node1->p_data)) < (*(char *)(p_node2->p_data)))
    {
        if (NULL == p_temp3)
        {
            fprintf(stderr, "cll_merge: p_node NULL\n");
            return NULL;
        }

        p_temp1 = p_node1;
        p_temp3 = p_temp3->p_next;
        counter1++;
    }
    else
    {
        if (NULL == p_temp4)
        {
            fprintf(stderr, "cll_merge: p_node NULL\n");
            return NULL;
        }

        p_temp1 = p_node2;
        p_temp4 = p_temp4->p_next;
        counter2++;
    }

    //p_temp2 will hold the pointer to the beginning of the linked list.
    //p_temp1 will be used to attach the rest of the linked list.
    p_temp2 = p_temp1;

    p_temp2 = cll_merge_helper(p_temp1, p_temp2, p_temp3, p_temp4, size1, 
                               size2, counter1, counter2);

    return p_temp2;
}

/**
 * @brief Supplied head of linked list as opposed to cll context to enable
 * recursion.
 * 
 * @param p_node head of circularly linked list context.
 * @return node_t* sorted and merged node. Returns NULL if memory access
 * error occurs.
 */
static node_t *
cll_mergesort (node_t * p_node)
{
    if (NULL == p_node)
    {
        fprintf(stderr, "cll_mergesort: p_node NULL\n");
        return NULL;
    }

    int size = 1;
    node_t * p_temp1 = p_node;

    while (p_temp1->p_next != p_node)
    {
        size++;

        if (NULL == p_temp1)
        {
            fprintf(stderr, "cll_mergesort: p_node NULL\n");
            return NULL;
        }

        p_temp1 = p_temp1->p_next;
    }

    //conditional enables recursion for cll_mergesort.
    if (size > 1)
    {
        node_t * p_temp2 = p_node;
        node_t * p_temp3 = p_node;

        int size1 = size/2;
        int size2 = size - size1;
        for (int counter = 1;counter < (size1);counter++)
        {
            if (NULL == p_temp3)
            {
                fprintf(stderr, "cll_mergesort: p_node NULL\n");
                return NULL;
            }
            p_temp3 = p_temp3->p_next;
        }

        //p_temp2 is then used to make the first half of the linked list
        //circular and assigned to the first position. p_temp3 is assigned to
        //point at the second half of the linked list.
        if (NULL == p_temp3)
        {
            fprintf(stderr, "cll_mergesort: p_node NULL\n");
            return NULL;
        }
        p_temp2 = p_temp3;
        p_temp3 = p_temp3->p_next;
        p_temp2->p_next = p_node;
        p_temp2 = p_node;

        //The first half of the linked list is now another circularly linked
        //list. The second half of the list still points back to the original
        //HEAD. The function then counts through this loop to re-link the
        //circle.
        node_t * p_temp4 = p_temp3;

        while (p_temp3->p_next != p_node)
        {
            if (NULL == p_temp3)
            {
                fprintf(stderr, "cll_mergesort: p_node NULL\n");
                return NULL;
            }

            p_temp3 = p_temp3->p_next;
        }

        if (NULL == p_temp3)
        {
            fprintf(stderr, "cll_mergesort: p_node NULL\n");
            return NULL;
        }

        p_temp3->p_next = p_temp4;

        //p_temp2 is the first half of the input list and p_temp4 is the
        //second half of the input list.
        p_temp2 = cll_mergesort(p_temp2);
        p_temp4 = cll_mergesort(p_temp4);

        p_node = cll_merge(p_temp2, p_temp4, size1, size2);

        return p_node;
    }
    return p_node;
}

/**
 * @brief cll_sort sorts a linked list alphanumerically. This function will
 * type cast all data to char in order to compare. Will return NULL if error
 * occurs.
 * 
 * @param p_cll pointer to cll context.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
cll_sort (cll_t * p_cll)
{
    if (NULL == (p_cll))
    {
        fprintf(stderr, "cll_sort: p_cll NULL\n");
        return FAILURE;
    }

    p_cll->p_head = cll_mergesort(p_cll->p_head);

    if (NULL == p_cll->p_head)
    {
        return FAILURE;
    }
    else
    {
        return SUCCESS;
    }
}

//End of cll.c library source file.
