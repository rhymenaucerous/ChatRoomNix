#ifndef T_POOL_LIB
#define T_POOL_LIB

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include "../queue_lib/queue.h"

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

#ifndef CONTINUE

#define CONTINUE 1

#endif

#ifndef SHUTDOWN

#define SHUTDOWN 0

#endif

#ifndef IMMEDIATE

#define IMMEDIATE 0

#endif

#ifndef WAIT

#define WAIT 1

#endif

#ifndef NOT_RUNNING

#define NOT_RUNNING 0

#endif

#ifndef RUNNING

#define RUNNING 1

#endif

#ifndef MAX_THREADS

#define MAX_THREADS 50

#endif

/**
 * @brief Task structure for thread pool library.
 *
 * @param p_function pointer to user-supplied function.
 * @param p_arg pointer to user-supplied argument. 
 */
typedef struct task_t {
    void (*p_function)(void * p_arg);
    void * p_arg;
} task_t;

/**
 * @brief Thread pool context structure.
 * 
 * @param queue_access_mutex mutex for access to task queue.
 * @param queue_wait_cond p thread condition for waiting to dequeue. Each 
 * task submission will send a signal to waiting threads.
 * @param shutdown_cond p thread condition for initiating thread pool shutdown.
 * @param num_threads user-supplied number of threads.
 * @param shutdown flag for shutdown processes.
 * @param queue_shutdown flag for shutdown processes.
 * @param p_threads pointer to thread list.
 * @param p_task_queue queue created using queue_lib, holds all submitted 
 * tasks.
 */
typedef struct t_pool_t {
    pthread_mutex_t queue_access_mutex;
    pthread_cond_t queue_wait_cond;
    pthread_cond_t shutdown_cond;
    uint8_t num_threads;
    uint8_t shutdown;
    uint8_t queue_shutdown;
    pthread_t * p_threads;
    queue_t * p_task_queue;
    //Mutexes for use in the chat room server implementation
    pthread_mutex_t users_mutex;
    pthread_mutex_t rooms_mutex;
} t_pool_t;

/**
 * @brief Initiates the thread pool context structure.
 * 
 * @param num_threads user-supplied number of threads.
 * @return t_pool_t* pointer to the thread pool context.
 */
t_pool_t *
t_pool_init (uint8_t * num_threads);

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
                                                        void * p_arg);

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
t_pool_destroy (t_pool_t * p_t_pool, int handle);

#endif //T_POOL_LIB

//End of t_pool.h library file
