#include "../include/cr_session_manager.h"

/**
 * @brief handles packets received from the client while in the chatting
 * state. calls users and chats libraries to handle leave, chat, and quit
 * requests.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_chatting tracker for whether the user is chatting or not.
 * @param p_user pointer to the logged in user's user_t struct.
 * @param p_logged_in tracker for whether the user is logged in or not.
 * @return int SUCCESS (0), FAILURE (1), CONNECTION_FAILURE (2), or
 * THREAD_SHUTDOWN (3).
 */
static int
cr_sm_chat_state (cr_package_t * p_cr_package, char * p_buffer,
          int * p_chatting, user_t * p_user, int * p_logged_in)
{
    if ((NULL == p_cr_package) || (NULL == p_buffer) || (NULL == p_chatting)
                                                        || (NULL == p_user))
    {
        fprintf(stderr, "cr_sm_chat_state: input NULL\n");
        return FAILURE;
    }

    received_msg_t p_recvd_msg;
    memset(&p_recvd_msg, 0, sizeof(received_msg_t));
    memcpy(&p_recvd_msg, p_buffer, sizeof(received_msg_t));

    int return_val;

    if ((CHAT_TYPE == p_recvd_msg.type) && (REQUEST == p_recvd_msg.opcode))
    {

        if (CHAT_STYPE == p_recvd_msg.s_type)
        {
            return_val = cr_chats_chat(p_cr_package->p_rooms, p_user,
                                                           p_buffer);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_chat_state: cr_chats_chat()\n");
            }

            return return_val;
        }
        else if (LEAVE_STYPE == p_recvd_msg.s_type)
        {
            return_val = cr_chats_leave(p_cr_package->p_rooms, p_chatting,
                      p_user, p_cr_package->p_ssl_holder->p_ssl, SEND_IT);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_chat_state: cr_chats_leave()\n");
            }

            return return_val;
        }
    }
    else if ((SESSION_TYPE == p_recvd_msg.type)  && (REQUEST ==
                                           p_recvd_msg.opcode))
    {
        if (QUIT_STYPE == p_recvd_msg.s_type)
        {
            if(FAILURE == cr_chats_leave(p_cr_package->p_rooms, p_chatting,
                     p_user, p_cr_package->p_ssl_holder->p_ssl, DONT_SEND))
            {
                fprintf(stderr, "cr_sm_chat_state: cr_chats_leave()\n");
                return FAILURE;
            }

            if (FAILURE == cr_users_logout(p_cr_package->p_users,
                p_cr_package->p_ssl_holder->p_ssl, p_user, p_logged_in,
                DONT_SEND))
            {
                fprintf(stderr, "cr_sm_chat_state: cr_users_logout()\n");
                return FAILURE;
            }

            return_val = cr_msg_send_ack(p_cr_package->p_ssl_holder->p_ssl,
                                                 SESSION_TYPE, QUIT_STYPE);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_chat_state: cr_msg_send_ack()\n");
                return return_val;
            }

            return THREAD_SHUTDOWN;
        }
    }

    //NOTE: If the received packet type is not login, register, or quit then
    //the server will simply return a fail packet.
    return_val = cr_msg_send_rej(p_cr_package->p_ssl_holder->p_ssl, FAIL_TYPE,
                                            FAIL_STYPE, INVALID_PACKET_RCODE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_sm_chat_state: cr_msg_send_rej()\n");
    }

    return return_val;

    return SUCCESS;
}

/**
 * @brief Helper function for cr_sm_logged_state. handles any packets of type
 * rooms.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_chatting tracker for whether the user is chatting or not.
 * @param p_user pointer to the logged in user's user_t struct.
 * @param p_recvd_msg mesage received from client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_sm_ls_rooms (cr_package_t * p_cr_package, char * p_buffer, int * p_chatting,
                                   user_t * p_user, received_msg_t p_recvd_msg)
{
    if ((NULL == p_cr_package) || (NULL == p_buffer) || (NULL == p_chatting) ||
                                                              (NULL == p_user))
    {
        fprintf(stderr, "cr_sm_ls_rooms: input NULL\n");
        return FAILURE;
    }

    int return_val;

    if (LIST_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_rooms_list(p_cr_package->p_rooms,
                             p_cr_package->p_ssl_holder);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_rooms: cr_rooms_list()\n");
        }

        return return_val;
    }
    else if (JOIN_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_rooms_join(p_cr_package->p_rooms,
            p_cr_package->p_ssl_holder, p_user, p_buffer, p_chatting);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_rooms: cr_rooms_join()\n");
        }

        return return_val;
    }
    else if (CREATE_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_rooms_create(p_cr_package->p_rooms,
                p_cr_package->p_ssl_holder->p_ssl, p_user, p_buffer);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_rooms: cr_rooms_create()\n");
        }

        return return_val;
    }
    else if (DEL_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_rooms_delete(p_cr_package->p_rooms,
                p_cr_package->p_ssl_holder->p_ssl, p_user, p_buffer);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_rooms: cr_rooms_delete()\n");
        }

        return return_val;
    }

    return NO_MATCH;
}

/**
 * @brief Helper function for cr_sm_logged_state. handles any packets of type
 * account.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_logged_in tracker for whether the user is logged in or not.
 * @param p_user pointer to the logged in user's user_t struct.
 * @param p_recvd_msg mesage received from client.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_sm_ls_account (cr_package_t * p_cr_package, char * p_buffer,
            int * p_logged_in, user_t * p_user, received_msg_t p_recvd_msg)
{
    if ((NULL == p_cr_package) || (NULL == p_buffer) ||
        (NULL == p_logged_in) || (NULL == p_user))
    {
        fprintf(stderr, "cr_sm_ls_account: input NULL\n");
        return FAILURE;
    }

    int return_val;

    if (ADMIN_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_users_admin(p_cr_package->p_users,
            p_cr_package->p_ssl_holder->p_ssl, p_buffer, p_user, ADMIN);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_account: cr_users_admin()\n");
        }

        return return_val;
    }
    else if (ADMIN_REMOVE_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_users_admin(p_cr_package->p_users,
            p_cr_package->p_ssl_holder->p_ssl, p_buffer, p_user, NOT_ADMIN);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_account: cr_users_admin()\n");
        }

        return return_val;
    }
    else if (DEL_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_users_remove_user(p_cr_package->p_users,
                    p_cr_package->p_ssl_holder->p_ssl, p_buffer, p_user);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_account: cr_users_remove_user()\n");
        }

        return return_val;
    }
    else if (LOGOUT_STYPE == p_recvd_msg.s_type)
    {
        return_val = cr_users_logout(p_cr_package->p_users,
            p_cr_package->p_ssl_holder->p_ssl, p_user, p_logged_in, SEND_IT);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_sm_ls_account: cr_users_logout()\n");
        }

        return return_val;
    }

    return NO_MATCH;
}

/**
 * @brief handles packets received from the client while in the logged in
 * state. calls users and rooms libraries to handle admin, removal, logout,
 * room creation, room deletion, room list, and room join requests.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_logged_in tracker for whether the user is logged in or not.
 * @param p_chatting tracker for whether the user is chatting or not.
 * @param p_user pointer to the logged in user's user_t struct.
 * @return int SUCCESS (0), FAILURE (1), CONNECTION_FAILURE (2), or
 * THREAD_SHUTDOWN (3).
 */
static int
cr_sm_logged_state (cr_package_t * p_cr_package, char * p_buffer,
            int * p_logged_in, int * p_chatting, user_t * p_user)
{
    if ((NULL == p_cr_package) || (NULL == p_buffer) || (NULL == p_logged_in)
                                 || (NULL == p_chatting) || (NULL == p_user))
    {
        fprintf(stderr, "cr_sm_logged_state: input NULL\n");
        return FAILURE;
    }

    received_msg_t p_recvd_msg;
    memset(&p_recvd_msg, 0, sizeof(received_msg_t));
    memcpy(&p_recvd_msg, p_buffer, sizeof(received_msg_t));

    int return_val = SUCCESS;

    if ((ACCOUNT_TYPE == p_recvd_msg.type) && (REQUEST == p_recvd_msg.opcode))
    {
        return_val = cr_sm_ls_account(p_cr_package, p_buffer, p_logged_in,
                                                     p_user, p_recvd_msg);

        if (NO_MATCH != return_val)
        {
            return return_val;
        }
    }
    else if ((ROOMS_TYPE == p_recvd_msg.type)  && (REQUEST ==
                                           p_recvd_msg.opcode))
    {
        return_val = cr_sm_ls_rooms(p_cr_package, p_buffer, p_chatting, p_user,
                                                                  p_recvd_msg);

        if (NO_MATCH != return_val)
        {
            return return_val;
        }
    }
    else if ((SESSION_TYPE == p_recvd_msg.type)  && (REQUEST ==
                                           p_recvd_msg.opcode))
    {
        if (QUIT_STYPE == p_recvd_msg.s_type)
        {
            if (FAILURE == cr_users_logout(p_cr_package->p_users,
                p_cr_package->p_ssl_holder->p_ssl, p_user, p_logged_in,
                DONT_SEND))
            {
                fprintf(stderr, "cr_sm_logged_state: cr_users_logout()\n");
                return return_val;
            }

            return_val = cr_msg_send_ack(p_cr_package->p_ssl_holder->p_ssl,
                                                 SESSION_TYPE, QUIT_STYPE);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_logged_state: cr_msg_send_ack()\n");
                return return_val;
            }

            return THREAD_SHUTDOWN;
        }
    }

    //NOTE: If the received packet type is not login, register, or quit then
    //the server will simply reutrn a fail packet.
    return_val = cr_msg_send_rej(p_cr_package->p_ssl_holder->p_ssl, FAIL_TYPE,
                                            FAIL_STYPE, INVALID_PACKET_RCODE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_sm_logged_state: cr_msg_send_rej()\n");
    }

    return return_val;
}

/**
 * @brief handles packets received from the client while in the connected
 * state. calls user library to manage user login and register requests.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_buffer pointer to buffer with received message.
 * @param p_logged_in tracker for whether the user is logged in or not.
 * @param pp_user double pointer to hold a pointer to the user.
 * @return int SUCCESS (0), FAILURE (1), CONNECTION_FAILURE (2), or
 * THREAD_SHUTDOWN (3).
 */
static int
cr_sm_connected_state (cr_package_t * p_cr_package, char * p_buffer,
                               int * p_logged_in, user_t ** pp_user)
{
    if ((NULL == p_cr_package) || (NULL == p_logged_in))
    {
        fprintf(stderr, "cr_sm_connected_state: input NULL\n");
        return FAILURE;
    }

    received_msg_t p_recvd_msg;
    memset(&p_recvd_msg, 0, sizeof(received_msg_t));
    memcpy(&p_recvd_msg, p_buffer, sizeof(received_msg_t));

    int return_val = SUCCESS;

    if ((ACCOUNT_TYPE == p_recvd_msg.type) && (REQUEST == p_recvd_msg.opcode))
    {
        if (LOGIN_STYPE == p_recvd_msg.s_type)
        {
            return_val = cr_users_login(p_cr_package->p_users,
                p_cr_package->p_ssl_holder, p_buffer, pp_user, p_logged_in);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_connected_state: cr_users_login()\n");
            }

            return return_val;
        }
        else if (REGISTER_STYPE == p_recvd_msg.s_type)
        {
            return_val = cr_users_register(p_cr_package->p_users,
                              p_cr_package->p_ssl_holder->p_ssl, p_buffer);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_connected_state: "
                                "cr_users_register()\n");
            }

            return return_val;
        }
    }
    else if ((SESSION_TYPE == p_recvd_msg.type)  && (REQUEST ==
                                           p_recvd_msg.opcode))
    {
        if (QUIT_STYPE == p_recvd_msg.s_type)
        {
            return_val = cr_msg_send_ack(p_cr_package->p_ssl_holder->p_ssl,
                                                 SESSION_TYPE, QUIT_STYPE);

            if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
            {
                fprintf(stderr, "cr_sm_connected_state: cr_msg_send_ack()\n");
                return return_val;
            }

            return THREAD_SHUTDOWN;
        }
    }

    //NOTE: If the received packet type is not login, register, or quit then
    //the server will simply reutrn a fail packet.
    return_val = cr_msg_send_rej(p_cr_package->p_ssl_holder->p_ssl, FAIL_TYPE,
                                  FAIL_STYPE, INVALID_PACKET_RCODE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_sm_connected_state: cr_msg_send_rej()\n");
    }

    return return_val;
}

/**
 * @brief Shutdowns SSL and frees items created in listener function.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param pp_user double pointer to hold a pointer to the user.
 */
static void
cr_sm_session_clean_help (cr_package_t * p_cr_package, user_t ** pp_user)
{
    if (NULL != p_cr_package->p_ssl_holder->p_ssl)
    {
        SSL_shutdown(p_cr_package->p_ssl_holder->p_ssl);
        SSL_free(p_cr_package->p_ssl_holder->p_ssl);
    }
    close(p_cr_package->p_ssl_holder->client_fd);
    SSL_CTX_free(p_cr_package->p_ssl_holder->p_ssl_ctx);
    FREE(p_cr_package->p_ssl_holder);
    FREE(pp_user);
    FREE(p_cr_package);
}

/**
 * @brief Cleans the package and pp_user memory and closes the socket.
 * If the user is still in a chat room or logged in when this function is
 * called, the associated structures will have the necessary changes made.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @param p_chatting tracker for whether the user is chatting or not.
 * @param p_logged_in tracker for whether the user is logged in or not.
 * @param pp_user double pointer to hold a pointer to the user.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_sm_session_clean (cr_package_t * p_cr_package, int * p_chatting,
                               int * p_logged_in, user_t ** pp_user)
{
    if(*p_chatting == CHATTING)
    {
        if(FAILURE == cr_chats_leave(p_cr_package->p_rooms, p_chatting,
                        *pp_user, p_cr_package->p_ssl_holder->p_ssl, DONT_SEND))
        {
            fprintf(stderr, "cr_sm_session_clean: cr_chats_leave()\n");
            cr_sm_session_clean_help(p_cr_package, pp_user);
            return FAILURE;
        }
    }

    if (*p_logged_in == LOGGED_IN)
    {
        if (FAILURE == cr_users_logout(p_cr_package->p_users,
            p_cr_package->p_ssl_holder->p_ssl, *pp_user, p_logged_in, DONT_SEND))
        {
            fprintf(stderr, "cr_sm_session_clean: cr_users_logout()\n");
            cr_sm_session_clean_help(p_cr_package, pp_user);
            return FAILURE;
        }
    }

    cr_sm_session_clean_help(p_cr_package, pp_user);

    return SUCCESS;
}

/**
 * @brief Maintains session with client. Listens for client packets and
 * responds according to messaging protocols after conducting necessary
 * actions.
 *
 * @param p_cr_package pointer to package with client file descriptor,
 * users_t struct, and rooms_t struct.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_sm_session_manager (cr_package_t * p_cr_package)
{
    if (NULL == p_cr_package)
    {
        fprintf(stderr, "cr_sm_session_manager: input NULL\n");
        return FAILURE;
    }

    //NOTE: Variables below determine state of thread for client
    //communications. Set on stack to enable auto free.
    int logged_in = NOT_LOGGED_IN;
    int chatting = NOT_CHATTING;
    int * p_logged_in = &logged_in;
    int * p_chatting = &chatting;

    user_t ** pp_user = calloc(1, sizeof(user_t *));

    if (NULL == pp_user)
    {
        perror("cr_sm_session_manager: pp_user calloc");
        return FAILURE;
    }

    while (CONTINUE == server_interrupt)
    {
        char p_buffer[BUFF_SIZE + 1] = {0};
        int return_val = SSL_read(p_cr_package->p_ssl_holder->p_ssl, p_buffer,
                                                                   BUFF_SIZE);
        if (0 >= return_val)
        {
            //NOTE: The timeout on the socket has been set to 3 seconds.
            //That's the intended functionality to enable server shutdown.
            //However, the following errors are accounted for to try listening
            //again.
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno) ||
            (EINTR == errno) || (ETIMEDOUT == errno))
            {
                continue;
            }
            else
            {
                perror("cr_sm_session_manager: SSL_read:");
                SSL_free(p_cr_package->p_ssl_holder->p_ssl);
                p_cr_package->p_ssl_holder->p_ssl = NULL;
                cr_sm_session_clean(p_cr_package, p_chatting, p_logged_in,
                                                                 pp_user);
                return SUCCESS;
            }
        }

        //NOTE: If the client terminates the connection the recv return value
        //will be zero.
        if (0 == return_val)
        {
            fprintf(stderr, "cr_sm_session_manager: "
                    "client disconnected\n");
            break;
        }

        //NOTE: State determination made here. States: connected, logged in,
        //chatting.
        if (NOT_LOGGED_IN == *p_logged_in)
        {
            return_val = cr_sm_connected_state(p_cr_package, p_buffer,
                                                         p_logged_in, pp_user);
        }
        else if (NOT_CHATTING == *p_chatting)
        {
            return_val = cr_sm_logged_state(p_cr_package, p_buffer,
                                            p_logged_in, p_chatting, *pp_user);
        }
        else
        {
            return_val = cr_sm_chat_state(p_cr_package, p_buffer,
                              p_chatting, *pp_user, p_logged_in);
        }

        if (FAILURE == return_val)
        {
            fprintf(stderr, "cr_sm_session_manager: "
                    "handle_packet_connected()\n");
            signal_handler(SIGINT);
            break;
        }
        //NOTE: Thread failure doesn't require server shutdown -
        //if the client disconnects, the situation doesn't necessitate
        //total shutdown.
        else if (CONNECTION_FAILURE == return_val)
        {
            fprintf(stderr, "cr_sm_session_manager: "
                    "handle_packet_connected()\n");
            break;
        }
        else if (THREAD_SHUTDOWN == return_val)
        {
            break;
        }
    }

    int return_val = cr_sm_session_clean(p_cr_package, p_chatting, p_logged_in,
                                                                      pp_user);

    return return_val;
}

//End of cr_session_manager.c file
