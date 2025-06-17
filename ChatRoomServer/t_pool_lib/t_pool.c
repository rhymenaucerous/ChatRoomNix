#include "t_pool.h"

//prototype function enabling t_pool_init
static void * t_pool_worker ();

/**
 * @brief Checks that the thread number given meets requirements: above zero
 * and lower than the number of cores on the system. This may still be an
 * excessive number of cores if less are allocated to the running function, but
 * limits thread count within reason.
 * 
 * @param num_threads user-supplied number of threads.
 * @return uint8_t SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
uint8_t t_pool_input_check (uint8_t * num_threads)
{
    if ((0 >= *num_threads) || (MAX_THREADS < *num_threads))
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Initializes mutexes and conditions associated with the fight 
 * arena library.
 * 
 * @param p_t_pool pointer to the thread pool structure.
 * @return int SUCCESS or FAILURE (0 or 1, respectively).
 */
static int
cr_init_helper (t_pool_t * p_t_pool)
{
    if (SUCCESS != pthread_mutex_init(&(p_t_pool->users_mutex), NULL))
    {
        perror("fighter_arena_init_helper: pthread_mutex_init:");
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_init(&(p_t_pool->rooms_mutex), NULL))
    {
        perror("fighter_arena_init_helper: pthread_mutex_init:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Initiates the thread pool context structure.
 * 
 * @param num_threads user-supplied number of threads.
 * @return t_pool_t* pointer to the thread pool context.
 */
t_pool_t *
t_pool_init (uint8_t * num_threads)
{
    if (FAILURE == t_pool_input_check(num_threads))
    {
        fprintf(stderr, "t_pool_init: Invalid number of threads requested\n");
        return NULL;
    }
    
    t_pool_t * p_t_pool = calloc(1, sizeof(t_pool_t));

    if (NULL == p_t_pool)
    {
        perror("t_pool_init: t_pool_init calloc:");
        return NULL;
    }

    if (SUCCESS != pthread_mutex_init(&(p_t_pool->queue_access_mutex), NULL))
    {
        perror("t_pool_init: pthread_mutex_init:");
        return NULL;
    }

    if (SUCCESS != pthread_cond_init(&(p_t_pool->queue_wait_cond), NULL))
    {
        perror("t_pool_init: pthread_cond_init:");
        return NULL;
    }
    

    if (SUCCESS != pthread_cond_init(&(p_t_pool->shutdown_cond), NULL))
    {
        perror("t_pool_init: pthread_cond_init:");
        return NULL;
    }

    if (FAILURE == cr_init_helper(p_t_pool))
    {
        return NULL;
    }
    
    p_t_pool->num_threads = *num_threads;

    p_t_pool->p_threads = calloc(p_t_pool->num_threads, sizeof(pthread_t));

    if (NULL == p_t_pool->p_threads)
    {
        FREE(p_t_pool);
        perror("t_pool_init: p_threads calloc");
        return NULL;
    }
    
    p_t_pool->shutdown = CONTINUE;
    p_t_pool->queue_shutdown = CONTINUE;

        p_t_pool->p_task_queue = queue_init();

    if (NULL == p_t_pool->p_task_queue)
    {
        FREE(p_t_pool->p_threads);
        FREE(p_t_pool);
        fprintf(stderr, "t_pool_init: queue_init failure\n");
        return NULL;
    }
    
    for (uint8_t counter = 0; counter < p_t_pool->num_threads; counter++)
    {
        if (SUCCESS != pthread_create(&(p_t_pool->p_threads[counter]), NULL, 
                                                   t_pool_worker, p_t_pool))
        {
            perror("t_pool_init: pthread_create:");
            FREE(p_t_pool->p_threads);
            FREE(p_t_pool);
            return NULL;
        }
    }

    return p_t_pool;
}

/**
 * @brief Function that controls the work that the threads do. Utilizes the
 * task queue held in the thread pool context to recieve tasks.
 * 
 * @param p_t_pool pointer to thread pool context.
 * @return void* pointer return required for pthread create library function.
 */
static void *
t_pool_worker (t_pool_t * p_t_pool)
{
    if (NULL == p_t_pool)
    {
        fprintf(stderr, "t_pool_worker: p_t_pool NULL\n");
        return NULL;
    }

    do
    {
        if (NULL == p_t_pool)
        {
            fprintf(stderr, "t_pool_worker: p_t_pool NULL\n");
            return NULL;
        }

        if (SUCCESS != pthread_mutex_lock(&(p_t_pool->queue_access_mutex)))
        {
            perror("t_pool_worker: pthread_mutex_lock:");
            return NULL;
        }

        int queue_size_holder = queue_size(p_t_pool->p_task_queue);

        if (FAILURE_NEGATIVE == queue_size_holder)
        {
            fprintf(stderr, "t_pool_worker: queue_size failure\n");
            return NULL;
        }

        while ((0 == queue_size_holder) && (SHUTDOWN != 
                                                    p_t_pool->shutdown))
        {
            if (SUCCESS != pthread_cond_signal(&(p_t_pool->shutdown_cond)))
            {
                perror("t_pool_worker: pthread_mutex_lock:");
                return NULL;
            }
            
            if (SUCCESS != pthread_cond_wait(&(p_t_pool->queue_wait_cond),
                                         &(p_t_pool->queue_access_mutex)))
            {
                perror("t_pool_worker: pthread_mutex_lock:");
                return NULL;
            }

            queue_size_holder = queue_size(p_t_pool->p_task_queue);

            if (FAILURE_NEGATIVE == queue_size_holder)
            {
                fprintf(stderr, "t_pool_worker: queue_size failure\n");
                return NULL;
            }
            }

        if (SHUTDOWN == p_t_pool->shutdown)
        {
            if (SUCCESS != pthread_mutex_unlock(
                                        &(p_t_pool->queue_access_mutex)))
            {
                perror("t_pool_worker: pthread_mutex_unlock:");
                return NULL;
            }
            break;
        }

        task_t * p_temp_task = queue_dequeue(p_t_pool->p_task_queue, NULL);
        
        if (NULL == p_temp_task)
        {
            fprintf(stderr, "t_pool_worker: queue_dequeue failure\n");
            return NULL;
        }

        if (SUCCESS != pthread_mutex_unlock(&(p_t_pool->queue_access_mutex)))
        {
            perror("t_pool_worker: pthread_mutex_unlock:");
            return NULL;
        }

        if (NULL == p_temp_task->p_function)
        {
            fprintf(stderr, "t_pool_worker: task function NULL\n");
            return NULL;
        }

        p_temp_task->p_function(p_temp_task->p_arg);
        FREE(p_temp_task);
    }
    while (CONTINUE == p_t_pool->shutdown);

    return NULL;
}

/**
 * @brief Submits a task to the thread pool task queue.
 * 
 * @param p_t_pool pointer to the thread pool context.
 * @param p_function user-supplied function to be handled by thread pool.
 * @param p_arg user-supplied argument to be utilized by given function.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
t_pool_submit_task (t_pool_t * p_t_pool, void (*p_function)(void *), 
                    void * p_arg)
{
    if ((NULL == p_t_pool) || (NULL == p_function))
    {
        fprintf(stderr, "t_pool_submit_task: input NULL\n");
        return FAILURE;
    }

    if (SHUTDOWN == p_t_pool->queue_shutdown)
    {
        fprintf(stderr, "t_pool_submit_task: shutdown processing\n");
        return FAILURE;
    }

    task_t * p_task = calloc(1, sizeof(task_t));

    if (NULL == p_task)
    {
        perror("t_pool_submit_task: p_task calloc");
        return FAILURE;
    }

    p_task->p_function = p_function;
    p_task->p_arg = p_arg;

    if (SUCCESS != pthread_mutex_lock(&(p_t_pool->queue_access_mutex)))
    {
        perror("t_pool_submit_task: pthread_mutex_lock:");
        FREE(p_task);
        return FAILURE;
    }

    if (FAILURE == queue_enqueue(p_t_pool->p_task_queue, p_task))
    {
        fprintf(stderr, "t_pool_submit_task: queue_enqueue failure\n");
        FREE(p_task);
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_unlock(&(p_t_pool->queue_access_mutex)))
    {
        perror("t_pool_submit_task: pthread_mutex_unlock:");
        FREE(p_task);
        return FAILURE;
    }
    
    if (SUCCESS != pthread_cond_signal(&(p_t_pool->queue_wait_cond)))
    {
        perror("t_pool_submit_task: pthread_cond_signal:");
        FREE(p_task);
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Implements mechanisms for wait option in t_pool_destroy function.
 * 
 * @param p_t_pool pointer to thread pool context.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
static int
t_pool_destroy_wait (t_pool_t * p_t_pool)
{
    if (SUCCESS != pthread_mutex_lock(&(p_t_pool->queue_access_mutex)))
    {
        perror("t_pool_destroy: pthread_mutex_lock:");
        FREE(p_t_pool);
        return FAILURE;
    }

    int queue_size_holder = queue_size(p_t_pool->p_task_queue);

    if (FAILURE_NEGATIVE == queue_size_holder)
    {
        fprintf(stderr, "t_pool_worker: queue_size failure\n");
        return FAILURE;
    }

    while (0 != queue_size_holder)
    {
        if (SUCCESS != pthread_cond_wait(&(p_t_pool->shutdown_cond), 
                                            &(p_t_pool->queue_access_mutex)))
        {
            perror("t_pool_destroy: pthread_cond_wait:");
            FREE(p_t_pool);
            return FAILURE;
        }

        queue_size_holder = queue_size(p_t_pool->p_task_queue);

        if (FAILURE_NEGATIVE == queue_size_holder)
        {
            fprintf(stderr, "t_pool_worker: queue_size failure\n");
            return FAILURE;
        }
    }

    if (SUCCESS != pthread_mutex_unlock(&(p_t_pool->queue_access_mutex)))
    {
        perror("t_pool_destroy: pthread_mutex_unlock:");
        FREE(p_t_pool);
        return FAILURE;
    }

    p_t_pool->shutdown = SHUTDOWN;

    if (SUCCESS != pthread_cond_broadcast(&(p_t_pool->queue_wait_cond)))
    {
        perror("t_pool_destroy: pthread_cond_broadcast:");
        FREE(p_t_pool);
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Initializes mutexes and conditions associated with the fight 
 * arena library.
 * 
 * @param p_t_pool pointer to the thread pool structure.
 * @return int SUCCESS or FAILURE (0 or 1, respectively).
 */
static int
cr_destroy_helper (t_pool_t * p_t_pool)
{
    if (SUCCESS != pthread_mutex_destroy(&(p_t_pool->users_mutex)))
    {
        perror("fighter_arena_destroy_helper: pthread_mutex_destroy:");
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_destroy(&(p_t_pool->rooms_mutex)))
    {
        perror("fighter_arena_destroy_helper: pthread_mutex_destroy:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Closes task queue, joins all threads, and destroys thread pool
 * context.
 * 
 * @param p_t_pool pointer to thread pool context.
 * @param handle user-supplied argument for either waiting (WAIT (1)) or
 * immediately (IMMEDIATE (0)) joining threads.
 * @return int SUCCESS or FAILURE (0 or 1, respectively) returned.
 */
int
t_pool_destroy (t_pool_t * p_t_pool, int handle)
{
    p_t_pool->queue_shutdown = SHUTDOWN;
    
    if (NULL == p_t_pool)
    {
        fprintf(stderr, "t_pool_worker: p_t_pool NULL\n");
        return FAILURE;
    }

    if (WAIT == handle)
    {
        if (FAILURE == t_pool_destroy_wait(p_t_pool))
        {
            fprintf(stderr, "t_pool_destroy: t_pool_destroy_wait failure\n");
            return FAILURE;
        }
    }
    else
    {
        p_t_pool->shutdown = SHUTDOWN;
    }

    for (uint8_t counter = 0; counter < p_t_pool->num_threads; counter++)
    {
        if (SUCCESS != pthread_join((p_t_pool->p_threads[counter]), NULL))
        {
            perror("t_pool_destroy: pthread_join:");
            FREE(p_t_pool);
            return FAILURE;
        }
    }

    FREE(p_t_pool->p_threads);

    if (SUCCESS != pthread_mutex_destroy(&(p_t_pool->queue_access_mutex)))
    {
        perror("t_pool_init: pthread_mutex_destroy:");
        return FAILURE;
    }

    if (SUCCESS != pthread_cond_destroy(&(p_t_pool->queue_wait_cond)))
    {
        perror("t_pool_init: pthread_cond_destroy:");
        return FAILURE;
    }

    if (SUCCESS != pthread_cond_destroy(&(p_t_pool->shutdown_cond)))
    {
        perror("t_pool_init: pthread_cond_destroy:");
        return FAILURE;
    }

    if (SUCCESS != cr_destroy_helper(p_t_pool))
    {
        perror("t_pool_destroy: fighter_arena_destroy_helper:");
        return FAILURE;
    }

    if (FAILURE == queue_full_destroy(&(p_t_pool->p_task_queue), &free))
    {
        FREE(p_t_pool);
        fprintf(stderr, "t_pool_destroy: queue_full_destroy failure\n");
        return FAILURE;
    }
    
    FREE(p_t_pool);

    return SUCCESS;
}

//End of t_pool.c library file
