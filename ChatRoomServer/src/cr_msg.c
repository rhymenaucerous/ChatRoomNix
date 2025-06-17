#include "../include/cr_msg.h"

/**
 * @brief Creates a rejection packet.
 * 
 * @param type packet type.
 * @param sub_type packet sub type.
 * @param rej_code rejection code.
 * @return rejection_t* returns a pointer to the packet.
 */
rejection_t *
cr_msg_create_rej (uint8_t type, uint8_t sub_type, uint8_t rej_code)
{
    rejection_t * p_rejection = calloc(1, sizeof(rejection_t));

    if (NULL == p_rejection)
    {
        perror("cr_msg_create_rej: calloc");
        return NULL;
    }

    p_rejection->type = type;
    p_rejection->s_type = sub_type;
    p_rejection->opcode = REJECT;
    p_rejection->r_code = rej_code;

    return p_rejection;
}

/**
 * @brief Creates an acknowledge packet.
 * 
 * @param type packet type.
 * @param sub_type packet sub type.
 * @return acknowledge_t* returns a pointer to the packet.
 */
acknowledge_t *
cr_msg_create_ack (uint8_t type, uint8_t sub_type)
{
    acknowledge_t * p_acknowledge = calloc(1, sizeof(acknowledge_t));

    if (NULL == p_acknowledge)
    {
        perror("cr_msg_create_reg_ack: calloc");
        return NULL;
    }

    p_acknowledge->type = type;
    p_acknowledge->s_type = sub_type;
    p_acknowledge->opcode = ACKNOWLEDGE;

    return p_acknowledge;
}

/**
 * @brief Creates a chat update packet.
 * 
 * @param p_username username of the chat sender.
 * @param p_chat chat the will be sent to the client.
 * @return chat_t* returns a pointer to the packet. Returns NULL on
 * failure.
 */
chat_t *
cr_msg_create_update (char * p_username, char * p_chat)
{
    if ((NULL == p_username) || (NULL == p_chat))
    {
        fprintf(stderr, "cr_msg_create_update: input NULL\n");
        return NULL;
    }

    chat_t * p_chat_ack = calloc(1, sizeof(chat_t));

    if (NULL == p_chat_ack)
    {
        perror("cr_msg_create_reg_ack: calloc");
        return NULL;
    }

    p_chat_ack->type = CHAT_TYPE;
    p_chat_ack->s_type = CHAT_STYPE;
    p_chat_ack->opcode = ACKNOWLEDGE;
    char * p_carrot = ">";
    strncpy(p_chat_ack->p_chat, p_username, MAX_USERNAME_LENGTH);
    strncpy((p_chat_ack->p_chat + MAX_USERNAME_LENGTH), p_carrot, 1);
    strncpy((p_chat_ack->p_chat + MAX_USERNAME_LENGTH + 1), p_chat,
                                                     MAX_CHAT_LEN);

    return p_chat_ack;
}

/**
 * @brief sends a reject packet of specified type and sub type to the client.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param type packet type.
 * @param sub_type packet sub type.
 * @param rej_code rejection code.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_rej (SSL * p_ssl, uint8_t type, uint8_t sub_type,
                                         uint8_t reject_code)
{
    rejection_t * p_rejection = cr_msg_create_rej(type, sub_type,
                                                    reject_code);

    if (NULL == p_rejection)
    {
        fprintf(stderr, "cr_msg_send_reg_rej: cr_msg_create_rej()\n");
        return FAILURE;
    }

    int sent_bytes = SSL_write(p_ssl, p_rejection, sizeof(rejection_t));

    if (0 >= sent_bytes)
    {
        perror("cr_msg_send_reg_rej: SSL_write():");
        FREE(p_rejection);
        return CONNECTION_FAILURE;
    }

    FREE(p_rejection);

    return SUCCESS;
}

/**
 * @brief sends a reject packet of specified type and sub type to the client.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param type packet type.
 * @param sub_type packet sub type.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_ack (SSL * p_ssl, uint8_t type, uint8_t sub_type)
{
    acknowledge_t * p_acknowledge = cr_msg_create_ack(type, sub_type);

    if (NULL == p_acknowledge)
    {
        fprintf(stderr, "cr_msg_send_reg_ack: cr_msg_create_ack()\n");
        return FAILURE;
    }

    int sent_bytes = SSL_write(p_ssl, p_acknowledge, sizeof(acknowledge_t));

    if (0 >= sent_bytes)
    {
        perror("cr_msg_send_reg_ack: SSL_write():");
        FREE(p_acknowledge);
        return CONNECTION_FAILURE;
    }

    FREE(p_acknowledge);

    return SUCCESS;
}

/**
 * @brief sends a chat update packet to the client.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_username username of the chat sender.
 * @param p_chat chat message.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_update (SSL * p_ssl, char * p_username, char * p_chat)
{
    if ((NULL == p_username) || (NULL == p_chat))
    {
        fprintf(stderr, "cr_msg_send_update: input NULL\n");
        return FAILURE;
    }
    
    chat_t * p_chat_ack = cr_msg_create_update(p_username, p_chat);

    if (NULL == p_chat_ack)
    {
        fprintf(stderr, "cr_msg_send_update: cr_msg_create_update()\n");
        return FAILURE;
    }

    int sent_bytes = SSL_write(p_ssl, p_chat_ack, sizeof(chat_t));

    if (0 >= sent_bytes)
    {
        perror("cr_msg_send_update: SSL_write():");
        FREE(p_chat_ack);
        return CONNECTION_FAILURE;
    }

    FREE(p_chat_ack);

    return SUCCESS;
}

/**
 * @brief helper function for cr_msg_send_file_ack.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param type packet type.
 * @param sub_type packet sub type.
 * @param p_filename filename of file being sent.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_msg_send_file_ack_helper (SSL * p_ssl, uint8_t type, uint8_t sub_type,
                                                       char * p_filename)
{
    if (NULL == p_filename)
    {
        fprintf(stderr, "cr_msg_send_file_ack_helper: input NULL\n");
        return FAILURE;
    }
    
    int return_val = cr_msg_send_ack(p_ssl, type, sub_type);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_msg_send_file_ack_helper: cr_msg_send_rej()\n");
        return return_val;
    }

    struct stat file_info;

    if (SUCCESS != stat(p_filename, &file_info))
    {
        perror("cr_msg_send_file_ack_helper: stat");
        return FAILURE;
    }

    off_t file_size = file_info.st_size;

    int file_descriptor = open(p_filename, O_RDONLY);

    if (FAILURE_NEGATIVE == file_descriptor)
    {
        perror("cr_msg_send_file_ack_helper: open");
        return FAILURE;
    }

    char p_buffer[MAX_CHAT_FILE_SIZE + 1] = {0};

    if (FAILURE_NEGATIVE == read(file_descriptor, p_buffer, file_size))
    {
        perror("cr_msg_send_file_ack_helper: sendfile");
        close(file_descriptor);
        return FAILURE;
    }

    if (FAILURE_NEGATIVE == SSL_write(p_ssl, p_buffer, file_size))
    {
        perror("cr_msg_send_file_ack_helper: sendfile");
        close(file_descriptor);
        return FAILURE;
    }

    close(file_descriptor);

    return SUCCESS;
}

/**
 * @brief uses TCP cork and sendfile to send a file with a rooms/list/ack
 * header.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param client_fd client socket file descriptor.
 * @param type packet type.
 * @param sub_type packet sub type.
 * @param p_filename filename of file being sent.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_file_ack (SSL * p_ssl, int client_fd, uint8_t type,
                           uint8_t sub_type, char * p_filename)
{
    if (NULL == p_filename)
    {
        fprintf(stderr, "cr_msg_send_file_ack: input NULL\n");
        return FAILURE;
    }
    
    int optval = 1;
    
    if (FAILURE_NEGATIVE == setsockopt(client_fd, IPPROTO_TCP, TCP_CORK,
                                               &optval, sizeof(optval)))
    {
        perror("cr_msg_send_file_ack: setsockopt()\n");
        return FAILURE;
    }

    int return_val = cr_msg_send_file_ack_helper(p_ssl, type, sub_type,
                                                           p_filename);

    optval = 0;

    if (FAILURE_NEGATIVE == setsockopt(client_fd, IPPROTO_TCP, TCP_CORK,
                                               &optval, sizeof(optval)))
    {
        perror("cr_msg_send_file_ack: setsockopt()\n");
        return CONNECTION_FAILURE;
    }

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_msg_send_file_ack: cr_msg_send_rej()\n");
    }

    return return_val;
}

//End of cr_msg.c file
