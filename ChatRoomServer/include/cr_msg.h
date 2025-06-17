#ifndef CR_MSG
#define CR_MSG

#include "cr_shared.h"

//Packet Types
#define ROOMS_TYPE 0
#define ACCOUNT_TYPE 1
#define CHAT_TYPE 2
#define SESSION_TYPE 3
#define FAIL_TYPE 255

//Packet Sub-Type
#define JOIN_STYPE 0
#define LIST_STYPE 1
#define CREATE_STYPE 2
#define REGISTER_STYPE 3
#define LOGIN_STYPE 4
#define ADMIN_STYPE 5
#define CHAT_STYPE 6
#define FAIL_STYPE 7
#define DEL_STYPE 8
#define ADMIN_REMOVE_STYPE 9
#define LEAVE_STYPE 10
#define LOGOUT_STYPE 11
#define QUIT_STYPE 12

//OPCODES
#define REQUEST 0
#define RESPONSE 1
#define REJECT 2
#define ACKNOWLEDGE 3
#define UPDATE 4

//Reject codes
#define SRV_BUSY_RCODE 0
#define SRV_ERR_RCODE 1
#define INVALID_PACKET_RCODE 2
#define USER_NAME_LEN 3
#define USER_NAME_CHAR 4
#define PASS_LEN 5
#define PASS_CHAR 6
#define USER_DOES_NOT_EXIST 7
#define INCORRECT_PASS 8
#define ADMIN_PRIV 9
#define USER_EXISTS 10
#define ROOM_EXISTS 11
#define USER_LOGGED_IN 12
#define ADMIN_SELF 13
#define MAX_USERS 14
#define MAX_CLIENTS 15
#define MAX_ROOMS 16
#define NO_ROOMS 17
#define ROOM_LEN 18
#define ROOM_CHARS 19
#define ROOM_DOES_NOT_EXIST 21
#define ROOM_IN_USE 22

#pragma pack(push, 1) //WARNING: Data structures are packed to ensure program
//network communications adhere to standards. Un-packing code could result in
//program incompatability with externalyl sourced programs.

//NOTE: Messages will not all be sent/received by the server. Some will only
//be sent and some only received. Comments below.

//NOTE: All messages will have type, sub-type, and opcode in the first three bytes.
//This mesage enables the receiving function to determine the type before
//handling the rest of the packet.

typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
} received_msg_t;

typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    uint8_t r_code;
} rejection_t;

typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
} acknowledge_t;

//Register packet.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_username[MAX_USERNAME_LENGTH + 1];
    char    p_password[MAX_PASSWORD_LENGTH + 1];
} register_req_t;

//Delete user packet.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_username[MAX_USERNAME_LENGTH + 1];
} delete_req_t;

//Login packets.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_username[MAX_USERNAME_LENGTH + 1];
    char    p_password[MAX_PASSWORD_LENGTH + 1];
} login_req_t;

//Admin packets.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_username[MAX_USERNAME_LENGTH + 1];
} admin_req_t;

//Room create packets.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_room_name[MAX_ROOM_NAME_LENGTH + 1];
} room_req_t;

//Room delete packets.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_room_name[MAX_ROOM_NAME_LENGTH + 1];
} room_d_req_t;

//Room Join packets.
typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_room_name[MAX_ROOM_NAME_LENGTH + 1];
} join_req_t;

typedef struct {
    uint8_t type;
    uint8_t s_type;
    uint8_t opcode;
    char    p_chat[MAX_USERNAME_LENGTH + MAX_CHAT_LEN + 2];
} chat_t;

#pragma pack(pop)

/**
 * @brief Creates a rejection packet.
 * 
 * @param type packet type.
 * @param sub_type packet sub type.
 * @param rej_code rejection code.
 * @return rejection_t* returns a pointer to the packet.
 */
rejection_t *
cr_msg_create_rej (uint8_t type, uint8_t sub_type, uint8_t rej_code);

/**
 * @brief Creates an acknowledge packet.
 * 
 * @param type packet type.
 * @param sub_type packet sub type.
 * @return acknowledge_t* returns a pointer to the packet.
 */
acknowledge_t *
cr_msg_create_ack (uint8_t type, uint8_t sub_type);

/**
 * @brief Creates a chat update packet.
 * 
 * @param p_username username of the chat sender.
 * @param p_chat chat the will be sent to the client.
 * @return chat_t* returns a pointer to the packet. Returns NULL on
 * failure.
 */
chat_t *
cr_msg_create_update (char * p_username, char * p_chat);

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
                                         uint8_t reject_code);

/**
 * @brief sends a reject packet of specified type and sub type to the client.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param type packet type.
 * @param sub_type packet sub type.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_ack (SSL * p_ssl, uint8_t type, uint8_t sub_type);

/**
 * @brief sends a chat update packet to the client.
 * 
 * @param p_ssl pointer to ssl socket file descriptor.
 * @param p_username username of the chat sender.
 * @param p_chat chat message.
 * @return int SUCCESS (0), FAILURE (1), or CONNECTION_FAILURE (2).
 */
int
cr_msg_send_update (SSL * p_ssl, char * p_username, char * p_chat);

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
                           uint8_t sub_type, char * p_filename);

#endif //CR_MSG

//End of cr_msg.h file