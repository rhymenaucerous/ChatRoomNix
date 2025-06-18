#ifndef CR_SHARED
#define CR_SHARED

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "../cll_lib/cll.h"
#include "../h_table_lib/h_table.h"
#include "../t_pool_lib/t_pool.h"
#include "../networking_lib/networking.h"
#include "../algorithms_lib/algorithms.h"

#ifndef SHARED_MACROS
#define SHARED_MACROS

#define FREE(a) \
    free(a); \
    (a) = NULL

//macros enabling clear returns from all functions.
#define SUCCESS 0
#define FAILURE 1
#define FAILURE_NEGATIVE -1

//NOTE: 512, 1024, 4096 are common buffer size chunks due to them being powers
//of 2, efficiently using memory. 1024 is a nice middle ground for potential
//packet sizes.
#define BUFF_SIZE 1024

#define CONTINUE 1
#define STOP 0

//filenames should not exceed 50 characters - this will enable some paths.
#define FILE_NAME_MAX_LEN 50

//Maximums for strings: IPv4 or IPv6 compatable.
#define HOST_MAX_STRING 40 //Max IP length is (IPv6) 40 -> 
                           //7 colons + 32 hexadecimal digits + terminator.

#define PORT_MAX_STRING 6 //Only numeric services allowed - max length of
                          //65535 is 5 + terminator.

//IP length + Port length - 1 (one less terminator) + 2 (: designator in code).
#define ADDR_MAX_STRING (HOST_MAX_STRING + PORT_MAX_STRING + 1)

#endif //SHARED_MACROS

//Username attributes
#define MAX_USERNAME_LENGTH 30
#define MIN_USERNAME_LENGTH 1

//Room name attributes
#define MAX_ROOM_NAME_LENGTH 30
#define MIN_ROOM_NAME_LENGTH 5
#define ROOM_ADDED_CHARS 12

//password attributes
#define MAX_PASSWORD_LENGTH 30
#define MIN_PASSWORD_LENGTH 5

//received chat attributes
#define MAX_CHAT_LEN 150
#define MIN_CHAT_LEN 1

//Server specific attribues
#define BACKLOG 5
#define MAX_TOTAL_USERS 100
#define MAX_TOTAL_CLIENTS 50
#define MIN_TOTAL_CLIENTS 2
#define MAX_TOTAL_ROOMS 20
#define MIN_TOTAL_ROOMS 1
#define MAX_CHAT_FILE_SIZE 1024

//Status code values
#define NOT_LOGGED_IN 0
#define LOGGED_IN 1
#define NOT_ADMIN 0
#define ADMIN 1
#define NOT_CHATTING 0
#define CHATTING 1

//Filenames
#define CONFIG_FILENAME "config.txt"
#define USER_FILENAME "users.txt"
#define USER_BACKUP_FILENAME "users_b.txt"
#define ROOM_NAME_LIST "rooms/room_names.log"
#define ROOM_NAME_LIST_BACKUP "rooms/room_names_b.log"
#define LOG_DIR "rooms"

//strtol MACRO
#define BASE10 10

//return values for caping users/rooms functionality
#define USERS_FULL 2
#define ROOMS_FULL 2
#define USER_PRESENT 3
#define ROOM_PRESENT 3

//Ranges for UTF-8 character encodings
#define UTF_LOWER_LETTER_MIN 97
#define UTF_LOWER_LETTER_MAX 122
#define UTF_UP_LETTER_MIN 65
#define UTF_UP_LETTER_MAX 90
#define UTF_COLON 58
#define UTF_NUM_MIN 48
#define UTF_NUM_MAX 57
#define UTF_SPEC_CHAR_R1_MIN 33
#define UTF_SPEC_CHAR_R1_MAX 47
#define UTF_SPEC_CHAR_R2_MIN 59
#define UTF_SPEC_CHAR_R2_MAX 64
#define UTF_SPEC_CHAR_R3_MIN 123
#define UTF_SPEC_CHAR_R3_MAX 126

//return values for cr libraries
#define CONNECTION_FAILURE 2
#define THREAD_SHUTDOWN 3
#define BAD_CHAR 4
#define DONT_SEND 0
#define SEND_IT 1

//Table size check macro
#define EMPTY 0

//Shared structures section

typedef struct {
    char     p_host[HOST_MAX_STRING + 1];
    char     p_port[PORT_MAX_STRING + 1];
    uint8_t  max_rooms;
    uint8_t  max_client;
} config_info_t;

typedef struct {
    char          p_username[MAX_USERNAME_LENGTH + 1];
    char          p_password[MAX_PASSWORD_LENGTH + 1];
    char          p_chat_room[MAX_ROOM_NAME_LENGTH + 1];
    volatile int  login_status;
    volatile int  admin_status;
    ssl_socket_holder_t * p_ssl_holder;
} user_t;

typedef struct {
    char              p_room_name[MAX_ROOM_NAME_LENGTH + 1];
    char              p_room_location[MAX_ROOM_NAME_LENGTH + ROOM_ADDED_CHARS];
    cll_t *           p_users;
    pthread_mutex_t   room_mutex;
} room_t;

typedef struct {
    h_table_t *       p_users_table;
    pthread_mutex_t * p_users_mutex;
    uint8_t           user_count;
    uint8_t           client_count;
    uint8_t           max_client;
} users_t;

typedef struct {
    h_table_t *       p_rooms_table;
    pthread_mutex_t  * p_rooms_mutex;
    uint8_t           room_count;
    uint8_t           max_rooms;
} rooms_t;

typedef struct {
    rooms_t * p_rooms;
    users_t * p_users;
    ssl_socket_holder_t * p_ssl_holder;
} cr_package_t;


/**
 * @brief Simple function to check if an int variable is a valid port number.
 * 
 * @param p_port int pointer to port number.
 * @return int SUCCESS or FAILURE (0 or 1 respectively).
 */
int
port_range_check (long * p_port);

/**
 * @brief when supplied to the h_table_destroy function, the following function
 * will be used to free the memory of each entry in the hash table. 
 * 
 * @param p_room_entry_holder The input into this function is a pointer to an
 * hash table entry that is type room_t (the pointer is void though).
 */
void
free_rooms (void * p_room_entry_holder);

#endif //CR_SHARED

//End of cr_shared.c file
