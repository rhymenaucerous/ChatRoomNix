#ifndef CR_USERS
#define CR_USERS

#include "cr_shared.h"
#include "cr_msg.h"

/**
 * @brief Adds users from user.txt file to p_users struct (with hash table)
 * and sets their attributes.
 * 
 * @param p_users pointer to users_t struct.
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_users_start (users_t * p_users);

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
cr_users_register (users_t * p_users, SSL * p_ssl, char * p_buffer);

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
               char * p_buffer, user_t ** pp_user, int * p_logged_in);

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
                             user_t * p_user, int admin_set_to);

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
                            int * p_logged_in, int send_message);


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
                                                     user_t * p_user);

#endif //CR_USERS

//End of cr_users.h file
