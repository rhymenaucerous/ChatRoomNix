#include "../include/cr_rooms.h"

/**
 * @brief Sets TCP cork socket option and sends a packet header along with
 * the file with all of the room names in it.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_rooms_list_helper (rooms_t * p_rooms, ssl_socket_holder_t * p_ssl_holder)
{
    if (NULL == p_rooms)
    {
        fprintf(stderr, "cr_rooms_list_helper: input NULL\n");
        return FAILURE;
    }

    int return_val = SUCCESS;

    if (EMPTY == p_rooms->room_count)
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ROOMS_TYPE, LIST_STYPE,
                                                                NO_ROOMS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_list_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_msg_send_file_ack(p_ssl_holder->p_ssl,
            p_ssl_holder->client_fd, ROOMS_TYPE, LIST_STYPE, ROOM_NAME_LIST);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_list_helper: cr_msg_send_file_ack()\n");
    }

    return return_val;
}

/**
 * @brief Sends a list of the rooms present to the client.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_list (rooms_t * p_rooms, ssl_socket_holder_t * p_ssl_holder)
{
    if (NULL == p_rooms)
    {
        fprintf(stderr, "cr_rooms_list: input NULL\n");
        return FAILURE;
    }

    int return_val;
    
    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_list: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_rooms_list_helper(p_rooms, p_ssl_holder);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_list: pthread_mutex_unlock:");
        return FAILURE;
    }

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_list: cr_msg_send_rej()\n");
    }
    
    return return_val;
}


/**
 * @brief critical section for join functionality.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @param p_user pointer to current user struct.
 * @param p_room_name room name.
 * @param p_chatting pointer to tracker that identifies whether the user
 * is in a room or not.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_join_helper (rooms_t * p_rooms, ssl_socket_holder_t * p_ssl_holder,
                      user_t * p_user, char * p_room_name, int * p_chatting)
{
    if ((NULL == p_rooms) || (NULL == p_user) || (NULL == p_room_name) ||
        (NULL == p_chatting))
    {
        fprintf(stderr, "cr_rooms_join_helper: input NULL\n");
        return FAILURE;
    }

    int return_val = SUCCESS;

    room_t * p_room = h_table_return_entry(p_rooms->p_rooms_table,
                                                     p_room_name);

    if (NULL == p_room)
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ROOMS_TYPE,
                                    JOIN_STYPE, ROOM_DOES_NOT_EXIST);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_join_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (SUCCESS != pthread_mutex_lock(&p_room->room_mutex))
    {
        perror("cr_rooms_join_helper: pthread_mutex_lock:");
        return FAILURE;
    }

    if (FAILURE == cll_insert_element_end(p_room->p_users, p_user))
    {
        fprintf(stderr, "cr_rooms_join_helper: cll_insert_element_end()\n");
        return_val = FAILURE;
    }

    strncpy(p_user->p_chat_room, p_room_name, strlen(p_room_name));

    int return_val_2 = cr_msg_send_file_ack(p_ssl_holder->p_ssl,
                        p_ssl_holder->client_fd, ROOMS_TYPE, JOIN_STYPE,
                        p_room->p_room_location);

    char * p_joined_message = "User has joined the room";
                                                    
    int return_val_3 = cr_chats_chat_send(p_room, p_user, p_joined_message);

    if (SUCCESS != pthread_mutex_unlock(&p_room->room_mutex))
    {
        perror("cr_rooms_join_helper: pthread_mutex_unlock:");
        return FAILURE;
    }

    if (FAILURE == return_val)
    {
        return FAILURE;
    }
    
    if ((FAILURE == return_val_2) || (CONNECTION_FAILURE == return_val_2))
    {
        fprintf(stderr, "cr_rooms_join_helper: cr_msg_send_ack()\n");
    }

    if ((FAILURE == return_val_3) || (CONNECTION_FAILURE == return_val_3))
    {
        fprintf(stderr, "cr_rooms_join_helper: cr_chats_chat_send()\n");
    }

    *p_chatting = CHATTING;

    return return_val;
}

/**
 * @brief Adds a user to a room. Sends a reject packet if the room doesn't
 * exist.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @param p_user pointer to current user struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_chatting pointer to tracker that identifies whether the user
 * is in a room or not.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_join (rooms_t * p_rooms, ssl_socket_holder_t * p_ssl_holder,
                  user_t * p_user, char * p_buffer, int * p_chatting)
{
    if ((NULL == p_rooms) || (NULL == p_user) || (NULL == p_buffer) ||
        (NULL == p_chatting))
    {
        fprintf(stderr, "cr_rooms_join: input NULL\n");
        return FAILURE;
    }

    join_req_t join_req;
    memset(&join_req, 0, sizeof(join_req_t));
    memcpy(&join_req, p_buffer, sizeof(join_req_t));
    join_req.p_room_name[MAX_ROOM_NAME_LENGTH] = '\0';

    int return_val;

    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_join: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_rooms_join_helper(p_rooms, p_ssl_holder, p_user,
                                  join_req.p_room_name, p_chatting);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_join: pthread_mutex_unlock:");
        return FAILURE;
    }

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_join: cr_rooms_join_helper()\n");
    }

    return return_val;
}

/**
 * @brief Looks through a string for specified characters that are allowed in
 * this server's usernames and passwords.
 * 
 * WARNING: it's assumed that the string will have 30 chars of space allocated
 * to it. If there's less space allocated and the string is not NULL
 * terminated, this function can cause segfaults.
 * 
 * @param p_string input string.
 * @return int SUCCESS (0) if all chars in string are applicable. BAD_CHAR (4)
 * if there are any characters outside of the accepted set.
 */
static int
cr_rooms_chk_str_chars (char * p_string)
{
    if (NULL == p_string)
    {
        fprintf(stderr, "cr_rooms_chk_str_chars: input NULL\n");
    }

    int str_len = strnlen(p_string, MAX_USERNAME_LENGTH);

    for (int index = 0; index < str_len; index++)
    {
        if ((UTF_LOWER_LETTER_MIN <= p_string[index]) &&
            (UTF_LOWER_LETTER_MAX >= p_string[index]))
        {
            continue;
        }
        else if ((UTF_UP_LETTER_MIN <= p_string[index]) &&
                 (UTF_UP_LETTER_MAX >= p_string[index]))
        {
            continue;
        }
        else if ((UTF_NUM_MIN <= p_string[index]) &&
                 (UTF_NUM_MAX >= p_string[index]))
        {
            continue;
        }
        else
        {
            return BAD_CHAR;
        }
    }

    return SUCCESS;
}

/**
 * @brief When a room is added to the server, the function writes that room
 * name to the room name list file.
 * 
 * @param p_filename room name.
 * @return int 
 */
static int
cr_rooms_create_name_to_file (char * p_room_name)
{
    if (NULL == p_room_name)
    {
        fprintf(stderr, "cr_rooms_create_name_to_file: input NULL\n");
        return FAILURE;
    }
    
    FILE * file_pointer = fopen(ROOM_NAME_LIST, "a");

    if (NULL == file_pointer)
    {
        perror("cr_rooms_create_name_to_file: fopen");
        return FAILURE;
    }

    fputs(p_room_name, file_pointer);
    fputc('\n', file_pointer);

    if (EOF == fclose(file_pointer))
    {
        perror("cr_rooms_create_name_to_file: fclose:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Create a rooms struct, initiallizes its data structure and log file.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param room_req packet received from client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_create_helper_2 (rooms_t * p_rooms, SSL * p_ssl,
                                     room_req_t room_req)
{
    if (NULL == p_rooms)
    {
        fprintf(stderr, "cr_rooms_create_helper_2: input NULL\n");
        return FAILURE;
    }

    int return_val;

    char p_filename[MAX_ROOM_NAME_LENGTH + ROOM_ADDED_CHARS] = {0};
    snprintf(p_filename, (MAX_ROOM_NAME_LENGTH + ROOM_ADDED_CHARS),
                             "rooms/%s.log", room_req.p_room_name);

    FILE * file_pointer = fopen(p_filename, "w");

    if (NULL == file_pointer)
    {
        perror("cr_rooms_create_helper_2: fopen:");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_rooms_create_helper_2: fclose:");
        return FAILURE;
    }

    room_t * p_room = calloc(1, sizeof(room_t));

    if (NULL == p_room)
    {
        perror("cr_rooms_create_helper_2: p_room calloc");
        return FAILURE;
    }

    strncpy(p_room->p_room_location, p_filename, strlen(p_filename));
    strncpy(p_room->p_room_name, room_req.p_room_name,
                        strlen(room_req.p_room_name));

    if (SUCCESS != pthread_mutex_init(&p_room->room_mutex, NULL))
    {
        perror("cr_rooms_create_helper_2: pthread_mutex_init:");
        FREE(p_room);
        return FAILURE;
    }

    p_room->p_users = cll_init();

    if (NULL == p_room->p_users)
    {
        fprintf(stderr, "cr_rooms_create_helper_2: cll_init");
        pthread_mutex_destroy(&p_room->room_mutex);
        FREE(p_room);
        return FAILURE;
    }

    if (FAILURE == h_table_new_entry(p_rooms->p_rooms_table, p_room,
                                               p_room->p_room_name))
    {
        fprintf(stderr, "cr_rooms_create_helper_2: h_table_new_entry()\n");
        cll_destroy(&p_room->p_users, NULL);
        pthread_mutex_destroy(&p_room->room_mutex);
        FREE(p_room);
        return FAILURE;
    }

    if (FAILURE == cr_rooms_create_name_to_file(room_req.p_room_name))
    {
        return FAILURE;
    }

    p_rooms->room_count++;

    return_val = cr_msg_send_ack(p_ssl, ROOMS_TYPE, CREATE_STYPE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_create_helper_2: cr_msg_send_ack()\n");
    }

    return return_val;
}

/**
 * @brief Checks if the max number of rooms has been met or if the room
 * already exists, sends a rejection packet if either are true, and then
 * calls helper 2.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param room_req packet received from client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_create_helper (rooms_t * p_rooms, SSL * p_ssl, room_req_t room_req)
{
    if (NULL == p_rooms)
    {
        fprintf(stderr, "cr_rooms_create_helper: input NULL\n");
        return FAILURE;
    }

    int return_val;

    if (p_rooms->room_count >= p_rooms->max_rooms)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, CREATE_STYPE,
                                                          MAX_ROOMS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_create_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (NULL != h_table_return_entry(p_rooms->p_rooms_table,
                                      room_req.p_room_name))
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, CREATE_STYPE,
                                                        ROOM_EXISTS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_create_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_rooms_create_helper_2(p_rooms, p_ssl, room_req);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_create_helper: "
                        "cr_rooms_create_helper_2()\n");
    }

    return return_val;
}

/**
 * @brief Creates a room log and a room struct that the server will track.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_buffer pointer to buffer with received message.
 * @param p_user pointer to current user struct.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_create (rooms_t * p_rooms, SSL * p_ssl, user_t * p_user,
                                                 char * p_buffer)
{
    if ((NULL == p_rooms) || (NULL == p_buffer) || (NULL == p_user))
    {
        fprintf(stderr, "cr_rooms_create: input NULL\n");
        return FAILURE;
    }

    room_req_t room_req;
    memset(&room_req, 0, sizeof(room_req_t));
    memcpy(&room_req, p_buffer, sizeof(room_req_t));
    room_req.p_room_name[MAX_ROOM_NAME_LENGTH] = '\0';

    int return_val;

    //NOTE: Sends a reject packet if the user doesn't have admin status.
    if (ADMIN != p_user->admin_status)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, CREATE_STYPE,
                                                         ADMIN_PRIV);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_create: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_rooms_chk_str_chars(room_req.p_room_name);

    //NOTE: Sends a reject packet if the room name has charaters that aren't
    //allowed.
    if (BAD_CHAR == return_val)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, CREATE_STYPE,
                                                         ROOM_CHARS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_create: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    //NOTE: Sends a reject packet if the room name is too short.
    if (MIN_ROOM_NAME_LENGTH > strlen(room_req.p_room_name))
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, CREATE_STYPE,
                                                           ROOM_LEN);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_create: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_create: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_rooms_create_helper(p_rooms, p_ssl, room_req);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_create: pthread_mutex_unlock:");
        return FAILURE;
    }

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_create: cr_rooms_create_helper()\n");
    }

    return return_val;
}

/**
 * @brief Removes room from log file.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_room_name received room name from user.
 * @return int SUCCESS (0) or FAILURE (1)
 */
static int
cr_rooms_delete_h_file (rooms_t * p_rooms, char * p_room_name)
{
    if ((NULL == p_rooms) || (NULL == p_room_name))
    {
        fprintf(stderr, "cr_rooms_delete_h_file: input NULL\n");
        return FAILURE;
    }

    char line_buffer[MAX_ROOM_NAME_LENGTH + 1] = {0};

    FILE * file_pointer;
    FILE * file_pointer_2;

    file_pointer = fopen(ROOM_NAME_LIST, "r");
    file_pointer_2 = fopen(ROOM_NAME_LIST_BACKUP, "w+");

    if ((NULL == file_pointer) || (NULL == file_pointer_2))
    {
        perror("cr_rooms_delete_h_file: fopen");
        return FAILURE;
    }

    while (NULL != fgets(line_buffer, (sizeof(line_buffer) - 1), file_pointer))
    {
        if (SUCCESS != strncmp(line_buffer, p_room_name, strlen(p_room_name)))
        {
            fwrite(line_buffer, strlen(line_buffer), 1, file_pointer_2);
        }
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_rooms_delete_h_file: fclose:");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer_2))
    {
        perror("cr_rooms_delete_h_file: fclose:");
        return FAILURE;
    }
    
    if (FAILURE_NEGATIVE == rename(ROOM_NAME_LIST_BACKUP, ROOM_NAME_LIST))
    {
        perror("cr_rooms_delete_h_file: rename:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Releases memory and structures being utilized by the room struct.
 * 
 * @param p_room pointer to room_t struct.
 * @return int SUCCESS (0) or FAILURE (1)
 */
static int
cr_users_free_room (room_t * p_room)
{
    if (NULL == p_room)
    {
        fprintf(stderr, "cr_users_free_room: input NULL\n");
        return FAILURE;
    }

    if (FAILURE == cll_destroy(&p_room->p_users, NULL))
    {
        fprintf(stderr, "cr_users_free_room: cll_destroy()\n");
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_destroy(&p_room->room_mutex))
    {
        perror("cr_users_free_room: pthread_mutex_destroy:");
        return FAILURE;
    }

    if (SUCCESS != remove(p_room->p_room_location))
    {
        perror("cr_users_free_room: remove:");
        return FAILURE;
    }

    FREE(p_room);

    return SUCCESS;
}

/**
 * @brief Sends a reject packet if the room doesn't exist or already has
 * a user in it. Destroys hash table entry and removes room from log file.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_room_name received room name from user.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_rooms_delete_helper (rooms_t * p_rooms, SSL * p_ssl, char * p_room_name)
{
    if ((NULL == p_rooms) || (NULL == p_room_name))
    {
        fprintf(stderr, "cr_rooms_delete_helper: input NULL\n");
        return FAILURE;
    }

    int return_val;

    room_t * p_room = h_table_return_entry(p_rooms->p_rooms_table,
                                                     p_room_name);
    
    if (NULL == p_room)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, DEL_STYPE,
                                             ROOM_DOES_NOT_EXIST);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_delete_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (EMPTY != p_room->p_users->size)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, DEL_STYPE,
                                                     ROOM_IN_USE);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_delete_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (NULL == h_table_destroy_entry(p_rooms->p_rooms_table, p_room_name))
    {
        fprintf(stderr, "cr_rooms_delete_helper: h_table_destroy_entry()\n");
        return FAILURE;
    }

    if (FAILURE == cr_users_free_room(p_room))
    {
        fprintf(stderr, "cr_rooms_delete_helper: cr_users_free_room()\n");
        return FAILURE;
    }

    if (FAILURE == cr_rooms_delete_h_file(p_rooms, p_room_name))
    {
        fprintf(stderr, "cr_rooms_delete_helper: cr_rooms_delete_h_file()\n");
        return FAILURE;
    }

    p_rooms->room_count--;

    return_val = cr_msg_send_ack(p_ssl, ROOMS_TYPE, DEL_STYPE);
    
    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_delete_helper: cr_msg_send_ack()\n");
    }

    return return_val;
}

/**
 * @brief Deletes a room from the list of rooms.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_user pointer to current user struct.
 * @param p_buffer pointer to buffer with received message.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_delete (rooms_t * p_rooms, SSL * p_ssl, user_t * p_user,
                                                 char * p_buffer)
{
    if ((NULL == p_rooms) || (NULL == p_buffer) || (NULL == p_user))
    {
        fprintf(stderr, "cr_rooms_delete: input NULL\n");
        return FAILURE;
    }

    int return_val;
    
    if (ADMIN != p_user->admin_status)
    {
        return_val = cr_msg_send_rej(p_ssl, ROOMS_TYPE, DEL_STYPE,
                                                      ADMIN_PRIV);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_rooms_delete: cr_msg_send_rej()\n");
        }

        return return_val;
    }
    room_d_req_t room_d_req;
    memset(&room_d_req, 0, sizeof(room_d_req_t));
    memcpy(&room_d_req, p_buffer, sizeof(room_d_req_t));
    room_d_req.p_room_name[MAX_ROOM_NAME_LENGTH] = '\0';

    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_delete: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_rooms_delete_helper(p_rooms, p_ssl,
                                        room_d_req.p_room_name);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_rooms_delete: pthread_mutex_unlock:");
        return FAILURE;
    }
    
    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_rooms_delete: cr_msg_send_ack()\n");
    }

    return return_val;
}

/**
 * @brief Creates rooms log directory and file for holding room name list.
 * 
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_rooms_start ()
{
    if (SUCCESS != mkdir(LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR))
    {
        perror("cr_rooms_start: mkdir");
        return FAILURE;
    }

    FILE * file_pointer = fopen(ROOM_NAME_LIST, "w");

    if (NULL == file_pointer)
    {
        perror("cr_rooms_start: fopen");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_rooms_start: fclose:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Cleans rooms list file and removes room directories on server
 * shutdown.
 * 
 */
void
cr_rooms_clean ()
{
    if (SUCCESS != remove(ROOM_NAME_LIST))
    {
        perror("cr_rooms_clean: remove");
    }

    if(SUCCESS != rmdir(LOG_DIR))
    {
        perror("cr_rooms_clean: rmdir");
    }
}

//End of cr_rooms.c file
