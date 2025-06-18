#include "../include/cr_chats.h"

/**
 * @brief Rotates log file if it exceeds maximum size.
 * 
 * @param p_room pointer to room_t struct of file being rotated.
 * @return int SUCCESS (0) or FAILURe (1).
 */
static int
cr_chats_rotate_file (room_t * p_room)
{
    if (NULL == p_room)
    {
        fprintf(stderr, "cr_chats_rotate_file: input NULL\n");
        return FAILURE;
    }

    char p_room_location_b[MAX_ROOM_NAME_LENGTH + ROOM_ADDED_CHARS + 4] = {0};

    snprintf(p_room_location_b, (strlen(p_room->p_room_location) + 4),
                                   "%s.log", p_room->p_room_location);

    char line_buffer[MAX_USERNAME_LENGTH + MAX_CHAT_LEN + 2] = {0};

    FILE * file_pointer;
    FILE * file_pointer_2;

    file_pointer = fopen(p_room->p_room_location, "r");
    file_pointer_2 = fopen(p_room_location_b, "w+");

    if ((NULL == file_pointer) || (NULL == file_pointer_2))
    {
        perror("cr_chats_rotate_file: fopen");
        return FAILURE;
    }

    int size_tracker = 0;

    while (NULL != fgets(line_buffer, (sizeof(line_buffer) - 1), file_pointer))
    {
        if (size_tracker > (MAX_CHAT_FILE_SIZE/2))
        {
            fwrite(line_buffer, strlen(line_buffer), 1, file_pointer_2);
        }
        size_tracker += strlen(line_buffer);
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_chats_rotate_file: fclose:");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer_2))
    {
        perror("cr_chats_rotate_file: fclose:");
        return FAILURE;
    }
    
    if (FAILURE_NEGATIVE == rename(p_room_location_b, p_room->p_room_location))
    {
        perror("cr_chats_rotate_file: rename:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Enters the chat that the user has sent the message to, to the
 * log file.
 * 
 * @param p_room pointer to room the client is in.
 * @param p_username client username.
 * @param p_chat message received from the client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_chats_chat_file (room_t * p_room, char * p_username, char * p_chat)
{
    if ((NULL == p_room) || (NULL == p_username) || (NULL == p_chat))
    {
        fprintf(stderr, "cr_chats_chat_file: input NULL\n");
        return FAILURE;
    }

    FILE * file_pointer = fopen(p_room->p_room_location, "a");

    if (NULL == file_pointer)
    {
        perror("cr_chats_chat_file: fopen");
        return FAILURE;
    }

    fputs(p_username, file_pointer);
    fputc('>', file_pointer);
    fputs(p_chat, file_pointer);
    fputc('\n', file_pointer);

    if (EOF == fclose(file_pointer))
    {
        perror("cr_chats_chat_file: fclose:");
        return FAILURE;
    }

    struct stat file_info;

    if (SUCCESS != stat(p_room->p_room_location, &file_info))
    {
        perror("cr_chats_chat_file: stat");
        return FAILURE;
    }

    off_t file_size = file_info.st_size;

    if (file_size > MAX_CHAT_FILE_SIZE)
    {
        if (FAILURE == cr_chats_rotate_file (p_room))
        {
            fprintf(stderr, "cr_chats_chat_file: cr_chats_rotate_file()\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * @brief Sends the received chat to all other users in the chat room.
 * 
 * WARNING: Calling function must lock the room's mutex before use and
 * unlock after use.
 * 
 * @param p_room pointer to room the client is in.
 * @param p_user pointer to current user struct.
 * @param p_chat message received from the client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_chats_chat_send (room_t * p_room, user_t * p_user, char * p_chat)
{
    if ((NULL == p_room) || (NULL == p_user) || (NULL == p_chat))
    {
        fprintf(stderr, "cr_chats_chat_send: input NULL\n");
        return FAILURE;
    }

    int return_val;

    if (EMPTY == p_room->p_users->size)
    {
        return SUCCESS;
    }

    for (int user_num = 0; user_num < p_room->p_users->size; user_num++)
    {
        user_t * p_temp_user = cll_return_element(p_room->p_users, user_num);

        if (NULL == p_temp_user)
        {
            fprintf(stderr, "cr_chats_chat_send: cll_return_element\n");
            return FAILURE;
        }

        //NOTE: The message should be sent to all users in the room except
        //the sender.
        if (p_user != p_temp_user)
        {
            return_val = cr_msg_send_update(p_temp_user->p_ssl_holder->p_ssl,
                                                 p_user->p_username, p_chat);
            
            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_chats_chat_send: cr_msg_send_update()\n");
                return return_val;
            }
        }
    }

    return SUCCESS;
}

/**
 * @brief Upon receiving client chat packet, adds the chat to the log file
 * and sends it to all other user in the room.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_user pointer to current user struct.
 * @param p_buffer message received from the client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_chats_chat (rooms_t * p_rooms, user_t * p_user, char * p_buffer)
{
    if ((NULL == p_rooms) || (NULL == p_user) || (NULL == p_buffer))
    {
        fprintf(stderr, "cr_chats_chat: input NULL\n");
        return FAILURE;
    }

    chat_t chat_req;
    memset(&chat_req, 0, sizeof(chat_t));
    memcpy(&chat_req, p_buffer, sizeof(chat_t));
    chat_req.p_chat[MAX_CHAT_LEN] = '\0';

    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_chats_chat: pthread_mutex_lock:");
        return FAILURE;
    }

    room_t * p_room = h_table_return_entry(p_rooms->p_rooms_table,
                                             p_user->p_chat_room);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_chats_chat: pthread_mutex_unlock:");
        return FAILURE;
    }

    if (NULL == p_room)
    {
        fprintf(stderr, "cr_chats_chat: room not found error\n");
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_lock(&p_room->room_mutex))
    {
        perror("cr_chats_chat: pthread_mutex_lock:");
        return FAILURE;
    }

    int return_val = cr_chats_chat_file(p_room, p_user->p_username,
                                                  chat_req.p_chat);

    int return_val_2 = cr_chats_chat_send(p_room, p_user, chat_req.p_chat);

    if (SUCCESS != pthread_mutex_unlock(&p_room->room_mutex))
    {
        perror("cr_chats_chat: pthread_mutex_unlock:");
        return FAILURE;
    }

    if (FAILURE == return_val)
    {
        fprintf(stderr, "cr_chats_chat: cr_chats_chat_file()\n");
        return return_val;
    }

    if ((FAILURE == return_val_2) || (CONNECTION_FAILURE == return_val_2))
    {
        fprintf(stderr, "cr_chats_chat: cr_chats_chat_send()\n");
    }

    return return_val_2;
}

/**
 * @brief Find the position of the specified user.
 * 
 * @param p_cll pointer to cll context with room's users.
 * @param p_user specified user.
 * @return int position value or FAILURE_NEGATIVE (-1).
 */
static int
cr_chats_find_user (cll_t * p_cll, user_t * p_user)
{
    if ((NULL == p_cll) || (NULL == p_user))
    {
        fprintf(stderr, "cr_chats_find_user: input NULL\n");
        return FAILURE_NEGATIVE;
    }

    if (NULL == p_cll->p_head)
    {
        fprintf(stderr, "cr_chats_find_user: p_cll head NULL\n");
        return FAILURE_NEGATIVE;
    }

    int counter = 0;
    node_t * p_temp = p_cll->p_head;

    while (counter <= p_cll->size)
    {
        user_t * p_temp_user = p_temp->p_data;
        
        if (p_user == p_temp_user)
        {
            return counter;
        }

        counter++;

        if (NULL == p_temp->p_next)
        {
            fprintf(stderr, "cr_chats_find_user: p_cll node NULL\n");
            return FAILURE_NEGATIVE;
        }

        p_temp = p_temp->p_next;
    }

    return FAILURE_NEGATIVE;
}

/**
 * @brief finds a specified user within the room_t struct's list and removes
 * them.
 * 
 * @param p_room pointer to room_t struct.
 * @param p_user specified user.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_chats_leave_helper (room_t * p_room, user_t * p_user)
{
    if ((NULL == p_room) || (NULL == p_user))
    {
        fprintf(stderr, "cr_chats_leave_helper: input NULL\n");
        return FAILURE;
    }

    int position = cr_chats_find_user(p_room->p_users, p_user);

    if (FAILURE_NEGATIVE == position)
    {
        fprintf(stderr, "cr_chats_leave_helper: cr_chats_find_user\n");
        return FAILURE;
    }

    if (FAILURE == cll_remove_element(p_room->p_users, position, NULL))
    {
        fprintf(stderr, "cr_chats_leave_helper: cll_remove_element\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief removes the user from the specified chat room and sets the chatting
 * tracker to NOT_CHATTING.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_chatting pointer to tracker that identifies whether the user
 * is in a room or not.
 * @param p_user pointer to current user struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param send_message determines whether the function will return a message.
 * This is utilized in conjuction with the quit command, to leave and logout
 * before terminating the connection.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_chats_leave (rooms_t * p_rooms, int * p_chatting, user_t * p_user,
                                       SSL * p_ssl, int send_message)
{
    if ((NULL == p_rooms) || (NULL == p_chatting) || (NULL == p_user))
    {
        fprintf(stderr, "cr_chats_leave: input NULL\n");
        return FAILURE;
    }

    int return_val = SUCCESS;


    if (SUCCESS != pthread_mutex_lock(p_rooms->p_rooms_mutex))
    {
        perror("cr_chats_leave: pthread_mutex_lock:");
        return FAILURE;
    }

    room_t * p_room = h_table_return_entry(p_rooms->p_rooms_table,
                                             p_user->p_chat_room);

    if (SUCCESS != pthread_mutex_unlock(p_rooms->p_rooms_mutex))
    {
        perror("cr_chats_leave: pthread_mutex_unlock:");
        return FAILURE;
    }

    if (NULL == p_room)
    {
        fprintf(stderr, "cr_chats_leave: h_table_return_entry");
        return FAILURE;
    }

    if (SUCCESS != pthread_mutex_lock(&p_room->room_mutex))
    {
        perror("cr_chats_leave: pthread_mutex_lock:");
        return FAILURE;
    }
    
    return_val = cr_chats_leave_helper(p_room, p_user);

    char * left_message = "User has left the room";

    int return_val_2 = cr_chats_chat_send(p_room, p_user, left_message);

    if (SUCCESS != pthread_mutex_unlock(&p_room->room_mutex))
    {
        perror("cr_chats_leave: pthread_mutex_unlock:");
        return FAILURE;
    }

    //NOTE: Checks whether leave helper was successful or not. Mutex had to be
    //unlocked first.
    if (FAILURE == return_val)
    {
        fprintf(stderr, "cr_chats_leave: cr_chats_r_user_f_room\n");
        return FAILURE;
    }

    //NOTE: Checks success of chat update send.
    if ((FAILURE == return_val_2) || (CONNECTION_FAILURE == return_val_2))
    {
        fprintf(stderr, "cr_chats_leave: cr_msg_send_ack()\n");
    }

    //NOTE: Utilized by quit while in the chatting state. Quit ack will be sent
    //and, as such, no leave ack is required.
    if (SEND_IT == send_message)
    {
        return_val = cr_msg_send_ack(p_ssl, CHAT_TYPE, LEAVE_STYPE);
        
        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_chats_leave: cr_msg_send_ack()\n");
        }
    }

    memset(p_user->p_chat_room, 0, sizeof(p_user->p_chat_room));

    *p_chatting = NOT_CHATTING;
    
    return return_val;
}

//End of cr_chats.c file
