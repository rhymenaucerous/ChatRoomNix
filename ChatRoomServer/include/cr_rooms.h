#ifndef CR_ROOMS
#define CR_ROOMS

#include "cr_shared.h"
#include "cr_msg.h"
#include "cr_chats.h"

/**
 * @brief Sends a list of the rooms present to the client.
 * 
 * @param p_rooms pointer to rooms_t struct.
 * @param p_ssl_holder pointer to struct with SSL and client file descriptors.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_rooms_list (rooms_t * p_rooms, ssl_socket_holder_t * p_ssl_holder);

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
                 user_t * p_user, char * p_buffer, int * p_chatting);

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
                                                char * p_buffer);

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
                                                char * p_buffer);

/**
 * @brief Creates rooms log directory and file for holding room name list.
 * 
 * @return int SUCCESS (0) or FAILURE (1).
 */
int
cr_rooms_start ();

/**
 * @brief Cleans rooms list file and removes room directories on server
 * shutdown.
 * 
 */
void
cr_rooms_clean ();

#endif //CR_ROOMS

//End of cr_rooms.h file
