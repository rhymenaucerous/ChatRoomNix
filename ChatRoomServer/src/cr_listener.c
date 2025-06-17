#include "../include/cr_listener.h"

/**
 * @brief Waits for all threads to finish through thread pool destory and then
 * frees shared data structures being used in the chat room server.
 * 
 * @param p_users pointer to users_t structure. If input present, the following
 * input will be ignored.
 * @param p_users_table pointer to hash table structure with user_t structures
 * inside.
 * @param p_rooms pointer to rooms_t structure. If input present, the following
 * input will be ignored.
 * @param p_rooms_table pointer to hash table structure with room_t structures
 * inside.
 * @param p_t_pool pointer to thread pool structure.
 * @param rooms_clean int specifying whether to clean rooms or not. Cleaning
 * means removing room logs and should only be conducted if the logs have
 * already been created.
 */
static void
cr_listener_clean(users_t * p_users, h_table_t * p_users_table,
                  rooms_t * p_rooms, h_table_t * p_rooms_table,
                  t_pool_t * p_t_pool, int rooms_clean)
{
    //WARNING: t_pool_destroy frees mutexes inside of p_users and p_rooms,
    //do not double free below.
    if (NULL != p_t_pool)
    {
        t_pool_destroy(p_t_pool, WAIT);
    }

    if (NULL != p_users)
    {
        h_table_destroy(p_users->p_users_table, &free);
        FREE(p_users);
    }
    else if (NULL != p_users_table)
    {
        h_table_destroy(p_users_table, &free);
    }

    if(NULL != p_rooms)
    {
        h_table_destroy(p_rooms->p_rooms_table, &free_rooms);
        FREE(p_rooms);
    }
    else if (NULL != p_rooms_table)
    {
        h_table_destroy(p_rooms_table, &free_rooms);
    }

    if (CLEAN == rooms_clean)
    {
        cr_rooms_clean();
    }
}

/**
 * @brief Thread created by connection to server. Starts session using session
 * manager library, responsible for cleaning client file descriptor and freeing
 * p_cr_package.
 * 
 * @param p_cr_package_holder pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct. Must be void pointer type to be
 * compatable with pthread library.
 */
static void
cr_listener_thread (void * p_cr_package_holder)
{    
    if (NULL == p_cr_package_holder)
    {
        fprintf(stderr, "cr_listener_thread: input NULL\n");
        return;
    }

    cr_package_t * p_cr_package = p_cr_package_holder;

    if (FAILURE == cr_sm_session_manager(p_cr_package))
    {
        fprintf(stderr, "cr_listener_thread: cr_session_manager()\n");
        server_interrupt = STOP;
    }
}

/**
 * @brief Listens for connection attempts and begins a thread to handle client
 * communications.
 * 
 * @param p_config_info pointer to configuration information from main server.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_listener_listen (config_info_t * p_config_info, rooms_t * p_rooms,
                              users_t * p_users, t_pool_t * p_t_pool)
{
    if (NULL == p_config_info)
    {
        fprintf(stderr, "cr_listener_listen: input NULL\n");
        return FAILURE;
    }

    int socket_fd = n_listen(p_config_info->p_host,
                                p_config_info->p_port,
                                p_config_info->max_client);
    
    if (FAILURE_NEGATIVE == socket_fd)
    {
        fprintf(stderr, "cr_listener_listen: n_listen()\n");
        return FAILURE;
    }

    while (CONTINUE == server_interrupt)
    {
        cr_package_t * p_cr_package = calloc(1, sizeof(cr_package_t));

        if (NULL == p_cr_package)
        {
            perror("cr_listener_listen: p_cr_package calloc");
            close(socket_fd);
            return FAILURE;
        }

        p_cr_package->p_rooms = p_rooms;
        p_cr_package->p_users = p_users;

        ssl_socket_holder_t * p_ssl_holder = calloc(1,
                         sizeof(ssl_socket_holder_t));

        if (NULL == p_ssl_holder)
        {
            perror("cr_listener_listen: p_ssl_holder calloc");
            FREE(p_cr_package);
            close(socket_fd);
            return FAILURE;
        }

        int client_fd = n_accept(socket_fd, p_ssl_holder);

        if (FAILURE_NEGATIVE == client_fd)
        {
            fprintf(stderr, "cr_listener_listen: n_accept()\n");
            FREE(p_ssl_holder);
            FREE(p_cr_package);
            close(socket_fd);
            return FAILURE;
        }

        p_cr_package->p_ssl_holder = p_ssl_holder;

        if (FAILURE == t_pool_submit_task(p_t_pool, cr_listener_thread,
                                                         p_cr_package))
        {
            fprintf(stderr, "cr_listener_listen: t_pool_submit_task()\n");
            FREE(p_ssl_holder);
            FREE(p_cr_package);
            close(socket_fd);
            return FAILURE;
        }
    }

    close(socket_fd);

    return SUCCESS;
}

/**
 * @brief Creates shared structures and thread pool for the server to being listening.
 * Starts listening funciton.
 * 
 * @param p_config_info pointer to configuration information from main server.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_listener(config_info_t * p_config_info)
{
    if (NULL == p_config_info)
    {
        fprintf(stderr, "cr_listenter: input NULL\n");
        return FAILURE;
    }

    uint8_t num_threads = p_config_info->max_client + 1;

    t_pool_t * p_t_pool = t_pool_init(&num_threads);

    if (NULL == p_t_pool)
    {
        fprintf(stderr, "cr_listener: t_pool_init");
        return FAILURE;
    }

    uint8_t room_h_table_size = next_prime(p_config_info->max_rooms);
    uint8_t user_h_table_size = next_prime(p_config_info->max_client);

    h_table_t * p_rooms_table = h_table_init(room_h_table_size, NULL);

    if (NULL == p_rooms_table)
    {
        fprintf(stderr, "cr_listener: p_rooms_table init\n");
        cr_listener_clean(NULL, NULL, NULL, NULL, p_t_pool, DONT_CLEAN);
        return FAILURE;
    }

    rooms_t * p_rooms = calloc(1,sizeof(rooms_t));

    if (NULL == p_rooms)
    {
        perror("cr_listener: p_rooms calloc");
        cr_listener_clean(NULL, NULL, NULL, p_rooms_table, p_t_pool, 
                                                        DONT_CLEAN);
        return FAILURE;
    }

    p_rooms->p_rooms_table = p_rooms_table;
    p_rooms->p_rooms_mutex = &p_t_pool->rooms_mutex;
    p_rooms->room_count = 0;
    p_rooms->max_rooms = p_config_info->max_rooms;

    h_table_t * p_users_table = h_table_init(user_h_table_size, NULL);

    if (NULL == p_users_table)
    {
        fprintf(stderr, "cr_listener: p_users_table init\n");
        cr_listener_clean(NULL, NULL, p_rooms, NULL, p_t_pool,DONT_CLEAN);
        return FAILURE;
    }

    users_t * p_users = calloc(1,sizeof(users_t));

    if (NULL == p_users)
    {
        perror("cr_listener: p_users calloc");
        cr_listener_clean(NULL, p_users_table, p_rooms, NULL, p_t_pool,
                                                            DONT_CLEAN);
        return FAILURE;
    }

    p_users->p_users_table = p_users_table;
    p_users->p_users_mutex = &p_t_pool->users_mutex;
    p_users->user_count = 0;
    p_users->max_client = p_config_info->max_client;

    if (FAILURE == cr_users_start(p_users))
    {
        fprintf(stderr, "cr_listener: cr_users_start()\n");
        cr_listener_clean(p_users, NULL, p_rooms, NULL, p_t_pool, DONT_CLEAN);
        return FAILURE;
    }

    if (FAILURE == cr_rooms_start())
    {
        fprintf(stderr, "cr_listener: cr_rooms_start()\n");
        cr_listener_clean(p_users, NULL, p_rooms, NULL, p_t_pool, DONT_CLEAN);
        return FAILURE;
    }

    if (FAILURE == cr_listener_listen(p_config_info, p_rooms, p_users,
                                                            p_t_pool))
    {
        fprintf(stderr, "cr_listener: cr_listener_listen()\n");
        cr_listener_clean(p_users, NULL, p_rooms, NULL, p_t_pool, CLEAN);
        return FAILURE;
    }

    cr_listener_clean(p_users, NULL, p_rooms, NULL, p_t_pool, CLEAN);
    return SUCCESS;
}

//End of cr_listener.c file
