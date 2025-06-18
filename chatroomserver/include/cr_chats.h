#ifndef CR_CHATS
#define CR_CHATS

#include "cr_shared.h"
#include "cr_msg.h"

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
cr_chats_chat_send (room_t * p_room, user_t * p_user, char * p_chat);

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
cr_chats_chat (rooms_t * p_rooms, user_t * p_user, char * p_buffer);

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
                                      SSL * p_ssl, int send_message);

#endif //CR_CHATS

//End of cr_chats.h file
