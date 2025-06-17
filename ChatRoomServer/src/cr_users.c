#include "../include/cr_users.h"

/**
 * @brief Takes a buffer line from the users.txt file and copies it to the
 * user_t struct name and password. Only accepts UTF-8 chars in the ranges
 * specified.
 *
 * @param p_user pointer to user_t struct.
 * @param p_buffer pointer to buffer line from users.txt file.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_users_from_buf (user_t * p_user, char * p_buffer)
{
    if ((NULL == p_user) || (NULL == p_buffer))
    {
        fprintf(stderr, "cr_users_add_file_user: input NULL\n");
        return FAILURE;
    }

    char * copy_to = p_user->p_username;
    int counter = 0;
    int offset = 0;

    for (int index = 0; index < strlen(p_buffer); index++)
    {
        if (MAX_USERNAME_LENGTH < counter)
        {
            fprintf(stderr, "cr_users_from_buf: username or password too "
                                                                "long\n");
            return FAILURE;
        }

        if ((UTF_LOWER_LETTER_MIN <= p_buffer[index]) &&
            (UTF_LOWER_LETTER_MAX >= p_buffer[index]))
        {
            copy_to[index - offset] = p_buffer[index];
            counter++;
        }
        else if ((UTF_UP_LETTER_MIN <= p_buffer[index]) &&
                 (UTF_UP_LETTER_MAX >= p_buffer[index]))
        {
            copy_to[index - offset] = p_buffer[index];
            counter++;
        }
        else if ((UTF_NUM_MIN <= p_buffer[index]) &&
                 (UTF_NUM_MAX >= p_buffer[index]))
        {
            copy_to[index - offset] = p_buffer[index];
            counter++;
        }
        else if (((UTF_SPEC_CHAR_R1_MIN <= p_buffer[index]) &&
                  (UTF_SPEC_CHAR_R1_MAX >= p_buffer[index])) ||
                 ((UTF_SPEC_CHAR_R2_MIN <= p_buffer[index]) &&
                  (UTF_SPEC_CHAR_R2_MAX >= p_buffer[index])) ||
                 ((UTF_SPEC_CHAR_R3_MIN <= p_buffer[index]) &&
                  (UTF_SPEC_CHAR_R3_MAX >= p_buffer[index])))
        {
            copy_to[index - offset] = p_buffer[index];
            counter++;
        }
        else if (UTF_COLON == p_buffer[index])
        {
            copy_to = p_user->p_password;
            offset = index + 1;
            counter = 0;
        }
        else if ('\n' == p_buffer[index])
        {
            break;
        }
        else
        {
            fprintf(stderr, "cr_users_from_buf: invalid characters in "
                                                        "users.txt\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * @brief Takes a buffer line from the users.txt file and copies it to the
 * user_t struct name and password. Sets login status to NOT_LOGGED_IN (0)
 * and admin status if the name is admin (should be in users.txt file). During
 * server run, others can be set to admin by the admin. Checks if the name is
 * available.
 *
 * @param p_users pointer to users_t struct.
 * @param p_buffer pointer to buffer line from users.txt file.
 * @return int SUCCESS (0) or FAILURE (1). Can also return USERS_FULL (2) and
 * USER_PRESENT (3).
 */
static int
cr_users_add_file_user (users_t * p_users, char * p_buffer)
{
    if ((NULL == p_users) || (NULL == p_buffer))
    {
        fprintf(stderr, "cr_users_add_file_user: input NULL\n");
        return FAILURE;
    }

    if (p_users->user_count == MAX_TOTAL_USERS)
    {
        return USERS_FULL;
    }

    user_t * p_user = calloc(1, sizeof(user_t));

    if (NULL == p_user)
    {
        perror("cr_users_add_file_user: p_user calloc");
        return FAILURE;
    }

    p_user->login_status = NOT_LOGGED_IN;

    if (FAILURE == cr_users_from_buf(p_user, p_buffer))
    {
        fprintf(stderr, "cr_users_add_file_user: cr_users_from_buf()\n");
        FREE(p_user);
        return FAILURE;
    }

    if (NULL != h_table_return_entry(p_users->p_users_table,
                                        p_user->p_username))
    {
        FREE(p_user);
        return USER_PRESENT;
    }

    if (SUCCESS == strncmp(p_user->p_username, "admin", strlen(
                                          p_user->p_username)))
    {
        p_user->admin_status = ADMIN;
    }
    else
    {
        p_user->admin_status = NOT_ADMIN;
    }

    if (FAILURE == h_table_new_entry(p_users->p_users_table, p_user,
                                                p_user->p_username))
    {
        fprintf(stderr, "cr_users_add_file_user: h_table_new_entry()\n");
        FREE(p_user);
        return FAILURE;
    }

    p_users->user_count++;

    return SUCCESS;
}

/**
 * @brief Adds users from user.txt file to p_users struct (with hash table)
 * and sets their attributes.
 *
 * @param p_users pointer to users_t struct.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_users_start (users_t * p_users)
{
    //NOTE: Mutex usage not required here: no threads have been initiated yet.
    FILE * file_pointer;

    file_pointer = fopen(USER_FILENAME, "r");

    if (NULL == file_pointer)
    {
        perror("cr_users_start: fopen");
        return FAILURE;
    }

    int line_count = 0;
    char p_buffer[BUFF_SIZE + 1] = {0};

    while (fgets(p_buffer, BUFF_SIZE, file_pointer) != NULL)
    {
        int result = cr_users_add_file_user(p_users, p_buffer);

        if (USERS_FULL == result)
        {
            break;
        }

        if (FAILURE == result)
        {
            fprintf(stderr, "cr_users_start: cr_users_add_file_user()\n");
            fclose(file_pointer);
            return FAILURE;
        }

        line_count++;
    }

    fclose(file_pointer);

    return SUCCESS;
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
cr_users_chk_str_chars (char * p_string)
{
    if (NULL == p_string)
    {
        fprintf(stderr, "cr_users_chk_str_chars: input NULL\n");
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
        else if (((UTF_SPEC_CHAR_R1_MIN <= p_string[index]) &&
                  (UTF_SPEC_CHAR_R1_MAX >= p_string[index])) ||
                 ((UTF_SPEC_CHAR_R2_MIN <= p_string[index]) &&
                  (UTF_SPEC_CHAR_R2_MAX >= p_string[index])) ||
                 ((UTF_SPEC_CHAR_R3_MIN <= p_string[index]) &&
                  (UTF_SPEC_CHAR_R3_MAX >= p_string[index])))
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
 * @brief Check username and password to ensure that both meet the specified
 * requirements.
 *
 * @param p_username pointer to username string.
 * @param p_password pointer to password string.
 * @param user_count the number of users the server contains.
 * @return int SUCCESS (0) if all requirements are met. reason code from the
 * following otherwise: USER_NAME_LEN/PASS_LEN/USER_NAME_CHAR/PASS_CHAR.
 */
static int
cr_users_chk_usr_and_pass (char * p_username, char * p_password,
                                                 int user_count)
{
    if ((NULL == p_username) || (NULL == p_password))
    {
        fprintf(stderr, "cr_users_chk_usr_and_pass: input NULL\n");
        return FAILURE;
    }

    p_username[MAX_USERNAME_LENGTH] = '\0';
    p_password[MAX_PASSWORD_LENGTH] = '\0';

    if (strlen(p_username) < MIN_USERNAME_LENGTH)
    {
        return USER_NAME_LEN;
    }

    if (strlen(p_password) < MIN_PASSWORD_LENGTH)
    {
        return PASS_LEN;
    }

    if (BAD_CHAR == cr_users_chk_str_chars(p_username))
    {
        return USER_NAME_CHAR;
    }

    if (BAD_CHAR == cr_users_chk_str_chars(p_password))
    {
        return PASS_CHAR;
    }

    if (user_count >= MAX_TOTAL_USERS)
    {
        return MAX_USERS;
    }

    return SUCCESS;
}

/**
 * @brief Adds users to user.txt file.
 *
 * WANRING: All string checking has already been done by other functions
 * prior to inclusion in this one. usernames and passwords could not maintain
 * standard if not checked before using this function.
 *
 * @param p_users pointer to users_t structure.
 * @param p_userpass string with user:pass formatted.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_users_add_user_file (users_t * p_users, char * p_userpass)
{
    if ((NULL == p_users) || (NULL == p_userpass))
    {
        fprintf(stderr, "cr_users_add_user_file: input NULL\n");
        return FAILURE;
    }

    FILE * file_pointer;

    file_pointer = fopen(USER_FILENAME, "a");

    if (NULL == file_pointer)
    {
        perror("cr_users_add_user_file: fopen");
        return FAILURE;
    }

    if (EOF == fputc('\n', file_pointer))
    {
        perror("cr_users_add_user_file: fputc");
        return FAILURE;
    }

    if (EOF == fputs(p_userpass, file_pointer))
    {
        perror("cr_users_add_user_file: fputs");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_users_add_user_file: fclose");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Creates a user_t struct and adds it to the users_t hash table.
 *
 * @param p_users pointer to users_t structure.
 * @param p_username username string.
 * @param p_password password string.
 * @return int SUCCESS (0) or FAILURE (1).
 */
static int
cr_users_add_user_table (users_t * p_users, char * p_username,
                                            char * p_password)
{
    if ((NULL == p_users) || (NULL == p_username) || (NULL == p_password))
    {
        fprintf(stderr, "cr_users_add_user_table: input NULL\n");
        return FAILURE;
    }

    user_t * p_user = calloc(1, sizeof(user_t));

    if (NULL == p_user)
    {
        perror("cr_users_add_user_table: calloc");
        return FAILURE;
    }

    strncpy(p_user->p_username, p_username, strlen(p_username));
    strncpy(p_user->p_password, p_password, strlen(p_password));

    if (FAILURE == h_table_new_entry(p_users->p_users_table, p_user,
                                                p_user->p_username))
    {
        fprintf(stderr, "cr_users_add_user_table: h_table_new_entry()\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Helps handle register packets sent from the client. Adds the user
 * to the users.txt file and to the users_t hash table. Sends a register
 * acknowledge packet to the client.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param register_req packet received by client in register request format.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_users_reg_helper (users_t * p_users, SSL * p_ssl, register_req_t register_req)
{
    if (NULL == p_users)
    {
        fprintf(stderr, "cr_users_reg_helper: input NULL\n");
        return FAILURE;
    }

    char p_userpass[MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 4] = {0};

    snprintf(p_userpass, (MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 3),
              "%s:%s", register_req.p_username, register_req.p_password);

    int add_return_1 = SUCCESS;
    int add_return_2 = SUCCESS;

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_reg_helper: pthread_mutex_lock:");
        return FAILURE;
    }

    add_return_1 = cr_users_add_user_table(p_users, register_req.p_username,
                                                   register_req.p_password);

    add_return_2 = cr_users_add_user_file(p_users, p_userpass);

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_reg_helper: pthread_mutex_unlock:");

        if (FAILURE == add_return_2)
        {
            fprintf(stderr, "cr_users_reg_helper: cr_users_add_user_file()\n");
        }
        else if (FAILURE == add_return_1)
        {
            fprintf(stderr, "cr_users_reg_helper: "
                         "cr_users_add_user_table()\n");
        }

        return FAILURE;
    }

    if (FAILURE == add_return_2)
    {
        fprintf(stderr, "cr_users_reg_helper: cr_users_add_user_file()\n");
        return FAILURE;
    }
    else if (FAILURE == add_return_1)
    {
        fprintf(stderr, "cr_users_reg_helper: cr_users_add_user_table()\n");
        return FAILURE;
    }

    int return_val = cr_msg_send_ack(p_ssl, ACCOUNT_TYPE, REGISTER_STYPE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_reg_helper: cr_msg_send_ack()\n");
        return return_val;
    }

    return SUCCESS;
}

/**
 * @brief Handles register packets sent from the client. Checks if the
 * username and password are valid and if the user doesn't exist. Sends
 * register reject packets to the client if necessary.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_buffer pointer to buffer.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_users_register (users_t * p_users, SSL * p_ssl, char * p_buffer)
{
    if ((NULL == p_users) || (NULL == p_buffer))
    {
        fprintf(stderr, "cr_users_register: input NULL\n");
        return FAILURE;
    }

    register_req_t register_req;
    memset(&register_req, 0, sizeof(register_req_t));
    memcpy(&register_req, p_buffer, sizeof(register_req_t));
    register_req.p_username[MAX_USERNAME_LENGTH] = '\0';
    register_req.p_password[MAX_PASSWORD_LENGTH] = '\0';

    int return_val = SUCCESS;

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_register: pthread_mutex_lock:");
        return FAILURE;
    }

    user_t * p_user = h_table_return_entry(p_users->p_users_table,
                                         register_req.p_username);

    int user_count = p_users->user_count;

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_register: pthread_mutex_unlock:");
        return FAILURE;
    }

    if (NULL != p_user)
    {
        return_val =  cr_msg_send_rej(p_ssl, ACCOUNT_TYPE,
                                      REGISTER_STYPE, USER_EXISTS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_register: cr_msg_send_rej()\n");
        }

        return return_val;
    }
    else
    {
        return_val = cr_users_chk_usr_and_pass(register_req.p_username,
                                  register_req.p_password, user_count);
    }

    if(FAILURE == return_val)
    {
        fprintf(stderr, "cr_users_register: cr_users_chk_usr_and_pass");
        return FAILURE;
    }
    //NOTE: If the password doesn't meet specifications, the return_val will
    //hold the reason code that can be sent to the client.
    else if (SUCCESS != return_val)
    {
        return_val =  cr_msg_send_rej(p_ssl, ACCOUNT_TYPE,
                                      REGISTER_STYPE, return_val);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_register: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_users_reg_helper(p_users, p_ssl, register_req);

    if (FAILURE == return_val)
    {
        fprintf(stderr, "cr_users_register: cr_users_reg_helper()\n");
    }

    return return_val;
}

/**
 * @brief Critical section of code for manipulating the users table. Checks
 * for cleint number, if the username exists, if they're already logged in,
 * if the password is correct, and sends either an ack or a reject packet
 * to the client.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @param login_req packet received from the client.
 * @param pp_user double pointer to user_t struct to have specified user
 * assigned to it.
 * @param p_logged_in pointer to logged in specifier int.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_users_login_helper (users_t * p_users, ssl_socket_holder_t * p_ssl_holder,
                 login_req_t login_req, user_t ** pp_user, int * p_logged_in)
{
    if ((NULL == p_users) || (NULL == pp_user) || (NULL == p_logged_in))
    {
        fprintf(stderr, "cr_users_login_helper: input NULL\n");
        return FAILURE;
    }

    user_t * p_user = h_table_return_entry(p_users->p_users_table,
                                            login_req.p_username);

    int return_val;


    if (p_users->client_count >= p_users->max_client)
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ACCOUNT_TYPE,
                                             LOGIN_STYPE, MAX_CLIENTS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_login_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (NULL == p_user)
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ACCOUNT_TYPE,
                                     LOGIN_STYPE, USER_DOES_NOT_EXIST);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_login_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (LOGGED_IN == p_user->login_status)
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ACCOUNT_TYPE,
                                          LOGIN_STYPE, USER_LOGGED_IN);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_login_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    login_req.p_password[MAX_PASSWORD_LENGTH] = '\0';
    int p_user_password_len = strnlen(p_user->p_password, MAX_PASSWORD_LENGTH);
    int login_req_password_len = strnlen(login_req.p_password, MAX_PASSWORD_LENGTH);

    if ((p_user_password_len != login_req_password_len) ||
        (SUCCESS != strncmp(login_req.p_password, p_user->p_password,
        p_user_password_len)))
    {
        return_val = cr_msg_send_rej(p_ssl_holder->p_ssl, ACCOUNT_TYPE,
                                          LOGIN_STYPE, INCORRECT_PASS);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_login_helper: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_msg_send_ack(p_ssl_holder->p_ssl, ACCOUNT_TYPE,
                                                      LOGIN_STYPE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_login_helper: cr_msg_send_ack()\n");
        return return_val;
    }

    p_user->p_ssl_holder = p_ssl_holder;
    p_user->login_status = LOGGED_IN;
    *pp_user = p_user;
    p_users->client_count++;
    *p_logged_in = LOGGED_IN;

    return SUCCESS;
}

/**
 * @brief Logs user into chat room server. If the username doesn't exist, the
 * user is logged in, or the password is incorrect the function will send a
 * login reject packet to the client.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @param p_buffer pointer to buffer with received message.
 * @param pp_user double pointer to user_t struct to have specified user
 * assigned to it.
 * @param p_logged_in pointer to logged in specifier int.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_users_login (users_t * p_users, ssl_socket_holder_t * p_ssl_holder,
                char * p_buffer, user_t ** pp_user, int * p_logged_in)
{
    if ((NULL == p_users) || (NULL == p_buffer) || (NULL == pp_user) ||
                                                 (NULL == p_logged_in))
    {
        fprintf(stderr, "cr_users_login: input NULL\n");
        return FAILURE;
    }

    login_req_t login_req;
    memset(&login_req, 0, sizeof(login_req_t));
    memcpy(&login_req, p_buffer, sizeof(login_req_t));
    login_req.p_username[MAX_USERNAME_LENGTH] = '\0';
    login_req.p_password[MAX_PASSWORD_LENGTH] = '\0';

    int return_val = SUCCESS;

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_login: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_users_login_helper(p_users, p_ssl_holder, login_req,
                                                  pp_user, p_logged_in);

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_login: pthread_mutex_unlock:");
        return FAILURE;
    }

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_login: cr_msg_send_ack()\n");
        return return_val;
    }

    return SUCCESS;
}

/**
 * @brief Critical section that inspects the hash table for the specified user.
 * Checks the specified user for logged in status and sets admin status to
 * ADMIN.
 *
 * @param p_users pointer to users_t struct.
 * @param p_username received username within user's admin packet.
 * @param admin_set_to either ADMIN or NOT_ADMIN. function can be used to set
 * a user to either.
 * @return int SUCCESS (0), FAILURE (1), or reason codes:
 * USER_DOES_NOT_EXIST (7) or USER_LOGGED_IN (12).
 */
static int
cr_users_admin_helper_2 (users_t * p_users, char * p_username,
                                           int admin_set_to)
{
    if (NULL == p_users)
    {
        fprintf(stderr, "cr_users_admin_helper_2: input NULL\n");
        return FAILURE;
    }

    user_t * p_temp_user = h_table_return_entry(p_users->p_users_table,
                                                           p_username);

    if (NULL == p_temp_user)
    {
        return USER_DOES_NOT_EXIST;
    }

    if (LOGGED_IN == p_temp_user->login_status)
    {
        return USER_LOGGED_IN;
    }

    p_temp_user->admin_status = admin_set_to;

    return SUCCESS;
}

/**
 * @brief Helper to cr_users_admin. Locks mutex and calls helper_2. Sends the
 * client either a rejection or acknowledge packet.
 *
 * @param p_users pointer to users_t struct.
 * @param admin_req received admin request packet from the client.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param admin_set_to either ADMIN or NOT_ADMIN.
 * @param sub_type specified sub-type: ADMIN_STYPE/ADMIN_REMOVE_STYPE.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_users_admin_helper_1 (users_t * p_users, admin_req_t admin_req,
                         SSL * p_ssl, int admin_set_to, int sub_type)
{
    if (NULL == p_users)
    {
        fprintf(stderr, "cr_users_admin_helper_1: input NULL\n");
        return FAILURE;
    }

    int return_val;

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_admin_helper_1: pthread_mutex_lock:");
        return FAILURE;
    }

    //NOTE: return value from helper will either be SUCCESS (0), FAILURE (1)
    //or the reason code.
    return_val = cr_users_admin_helper_2(p_users, admin_req.p_username,
                                                       admin_set_to);

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_admin_helper_1: pthread_mutex_unlock:");

        if (FAILURE == return_val)
        {
            fprintf(stderr, "cr_users_admin_helper_1:"
                       "cr_users_admin_helper_2()\n");
        }

        return FAILURE;
    }

    if (FAILURE == return_val)
    {
        fprintf(stderr, "cr_users_admin_helper_1:"
                   "cr_users_admin_helper_2()\n");
        return FAILURE;
    }

    if (SUCCESS != return_val)
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, sub_type,
                                                           return_val);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_admin_helper_1: "
                                "cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_msg_send_ack(p_ssl, ACCOUNT_TYPE, sub_type);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_admin_helper_1: "
                            "cr_msg_send_ack()\n");
    }

    return return_val;
}

/**
 * @brief Sets a specified user's admin status to ADMIN if the user exists
 * and is not logged in. Verifies user requesting update is admin and not
 * updating their own status.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_buffer pointer to buffer with received message.
 * @param p_user pointer to current user struct.
 * @param admin_set_to either ADMIN or NOT_ADMIN. function can be used to set
 * a user to either.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_users_admin (users_t * p_users, SSL * p_ssl, char * p_buffer,
                              user_t * p_user, int admin_set_to)
{
    if ((NULL == p_users) || (NULL == p_buffer) || (NULL == p_user))
    {
        fprintf(stderr, "cr_users_admin: input NULL\n");
        return FAILURE;
    }

    int sub_type = ADMIN_STYPE;

    if (ADMIN == admin_set_to)
    {
        sub_type = ADMIN_STYPE;
    }
    else
    {
        sub_type = ADMIN_REMOVE_STYPE;
    }

    admin_req_t admin_req;
    memset(&admin_req, 0, sizeof(admin_req_t));
    memcpy(&admin_req, p_buffer, sizeof(admin_req_t));
    admin_req.p_username[MAX_USERNAME_LENGTH] = '\0';

    int return_val = SUCCESS;

    if (SUCCESS == strncmp(p_user->p_username, admin_req.p_username,
                                               MAX_USERNAME_LENGTH))
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, sub_type,
                                                           ADMIN_SELF);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_admin: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (NOT_ADMIN == p_user->admin_status)
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, sub_type,
                                                           ADMIN_PRIV);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_admin: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    return_val = cr_users_admin_helper_1(p_users, admin_req, p_ssl,
                                               admin_set_to, sub_type);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_admin: cr_users_admin_helper_1()\n");
    }

    return return_val;
}

/**
 * @brief Logs user out of the server by placing the p_user login status
 * attribute to NOT_LOGGED_IN and setting the int pointer to that as well.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_user pointer to current user struct.
 * @param p_logged_in pointer to logged in specifier int.
 * @param send_message determines whether the function will return a message.
 * This is utilized in conjuction with the quit command, to logout before
 * terminating the connection.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_users_logout (users_t * p_users, SSL * p_ssl, user_t * p_user,
                             int * p_logged_in, int send_message)
{
    if ((NULL == p_users) || (NULL == p_user) || (NULL == p_logged_in))
    {
        fprintf(stderr, "cr_users_logout: input NULL\n");
        return FAILURE;
    }

    if (SEND_IT == send_message)
    {
        int return_val = cr_msg_send_ack(p_ssl, ACCOUNT_TYPE, LOGOUT_STYPE);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_logout: cr_msg_send_ack()\n");
            return return_val;
        }
    }

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_logout: pthread_mutex_lock:");
        return FAILURE;
    }

    p_user->login_status = NOT_LOGGED_IN;
    p_users->client_count--;

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_logout: pthread_mutex_unlock:");
        return FAILURE;
    }

    *p_logged_in = NOT_LOGGED_IN;

    return SUCCESS;
}

/**
 * @brief Removes specified user from the user table.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_username username being deleted.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_users_remove_table (users_t * p_users, SSL * p_ssl, char * p_username)
{
    if ((NULL == p_users) || (NULL == p_username))
    {
        fprintf(stderr, "cr_users_remove_table: input NULL\n");
        return FAILURE;
    }

    int return_val;

    user_t * p_user = h_table_return_entry(p_users->p_users_table,
                                                      p_username);

    if (NULL == p_user)
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, DEL_STYPE,
                                               USER_DOES_NOT_EXIST);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_remove_table: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (LOGGED_IN == p_user->login_status)
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, DEL_STYPE,
                                                        USER_LOGGED_IN);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_remove_table: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (NULL == h_table_destroy_entry(p_users->p_users_table, p_username))
    {
        fprintf(stderr, "cr_users_remove_table: cr_msg_send_rej()\n");
        return FAILURE;
    }

    return_val = cr_msg_send_ack(p_ssl, ACCOUNT_TYPE, DEL_STYPE);

    if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
    {
        fprintf(stderr, "cr_users_remove_table: cr_msg_send_ack()\n");
    }

    FREE(p_user);
    return return_val;
}

/**
 * @brief Removes specified user from the users.txt file.
 *
 * @param p_users pointer to users_t struct.
 * @param p_username username being deleted.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
static int
cr_users_remove_file (users_t * p_users, char * p_username)
{
    if ((NULL == p_users) || (NULL == p_username))
    {
        fprintf(stderr, "cr_users_remove_file: input NULL\n");
        return FAILURE;
    }

    //NOTE: The two additional buffer spaces are for the colon and newline.
    char line_buffer[(MAX_USERNAME_LENGTH + MAX_PASSWORD_LENGTH + 2)] = {0};

    FILE * file_pointer;
    FILE * file_pointer_2;

    file_pointer = fopen(USER_FILENAME, "r");
    file_pointer_2 = fopen(USER_BACKUP_FILENAME, "w+");

    if ((NULL == file_pointer) || (NULL == file_pointer_2))
    {
        perror("cr_users_remove_file: fopen");
        return FAILURE;
    }

    while (NULL != fgets(line_buffer, (sizeof(line_buffer) - 1), file_pointer))
    {
        if (SUCCESS != strncmp(line_buffer, p_username, strlen(p_username)))
        {
            fwrite(line_buffer, strlen(line_buffer), 1, file_pointer_2);
        }
    }

    if (EOF == fclose(file_pointer))
    {
        perror("cr_users_remove_file: fclose:");
        return FAILURE;
    }

    if (EOF == fclose(file_pointer_2))
    {
        perror("cr_users_remove_file: fclose:");
        return FAILURE;
    }

    if (FAILURE_NEGATIVE == rename(USER_BACKUP_FILENAME, USER_FILENAME))
    {
        perror("cr_users_remove_file: rename:");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * @brief Removes a user from the server.
 *
 * @param p_users pointer to users_t struct.
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_buffer pointer to buffer with received message.
 * @param p_user pointer to current user struct.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_users_remove_user (users_t * p_users, SSL * p_ssl, char * p_buffer,
                                                      user_t * p_user)
{
    if ((NULL == p_users) || (NULL == p_buffer) || (NULL == p_user))
    {
        fprintf(stderr, "cr_users_remove_user: input NULL\n");
        return FAILURE;
    }

    int return_val;
    int return_val_2;

    if (NOT_ADMIN == p_user->admin_status)
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, DEL_STYPE,
                                                            ADMIN_PRIV);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_remove_user: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    delete_req_t delete_req;
    memset(&delete_req, 0, sizeof(delete_req_t));
    memcpy(&delete_req, p_buffer, sizeof(delete_req_t));
    delete_req.p_username[MAX_USERNAME_LENGTH] = '\0';

    if (SUCCESS == strncmp(p_user->p_username, delete_req.p_username,
                                                MAX_USERNAME_LENGTH))
    {
        return_val = cr_msg_send_rej(p_ssl, ACCOUNT_TYPE, DEL_STYPE,
                                                            ADMIN_SELF);

        if ((FAILURE == return_val) || (CONNECTION_FAILURE == return_val))
        {
            fprintf(stderr, "cr_users_remove_user: cr_msg_send_rej()\n");
        }

        return return_val;
    }

    if (SUCCESS != pthread_mutex_lock(p_users->p_users_mutex))
    {
        perror("cr_users_remove_user: pthread_mutex_lock:");
        return FAILURE;
    }

    return_val = cr_users_remove_table(p_users, p_ssl, delete_req.p_username);

    return_val_2 = cr_users_remove_file(p_users, delete_req.p_username);

    if (SUCCESS != pthread_mutex_unlock(p_users->p_users_mutex))
    {
        perror("cr_users_remove_user: pthread_mutex_unlock:");
        return FAILURE;
    }

    if ((FAILURE == return_val) || (FAILURE == return_val_2))
    {
        fprintf(stderr, "cr_users_remove_user: cr_users_remove_table()/"
                                              "cr_users_remove_file\n");
        return FAILURE;
    }

    if ((CONNECTION_FAILURE == return_val) ||
        (CONNECTION_FAILURE == return_val_2))
    {
        fprintf(stderr, "cr_users_remove_user: cr_users_remove_table()/"
                                              "cr_users_remove_file\n");
        return CONNECTION_FAILURE;
    }

    return SUCCESS;
}

//End of cr_users.c file
