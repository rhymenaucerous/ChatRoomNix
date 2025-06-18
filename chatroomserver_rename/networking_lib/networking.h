#ifndef NETWORKING_LIB
#define NETWORKING_LIB

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

//NOTE: Added SSL libraries for chat room server.
#include <openssl/err.h> //errors
#include <openssl/ssl.h> //core library

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

typedef struct {
    SSL * p_ssl;
    SSL_CTX * p_ssl_ctx;
    int client_fd;
} ssl_socket_holder_t;

//Variable designated to handle SIGINT signal and enable graceful shutdown
//of waiting sockets.
extern volatile sig_atomic_t server_interrupt;

/**
 * @brief Given a hostname/IP address and port number, n_listen will use 
 * getaddrinfo() to create a list of possible sockaddr structs for connection
 * and attempt to listen on those settings (this increases the portability of
 * the code). n_listen will then use socket(), bind(), and listen() and then
 * return the listening file descriptor. n_listen sets SO_REUSEADDR (to enable
 * connections to recently closed sockets) and listening time to 3 seconds by
 * default.
 * 
 * @param p_address string with the host address in either letter or numeric
 * form.
 * @param p_port string with the port in numeric form.
 * @return int either the value of the file descriptor or FAILURE_NEGATIVE
 * (-1).
 */
int
n_listen (char * p_address, char * p_port, int backlog);

/**
 * @brief Handles SIGINT (cntrl+C) commands by user to set external variable
 * server_interrupt and allow graceful shutdown for the server.
 * 
 * @param signum Value of signal being recieved. Signal() usage in library
 * dictates this will only be SIGINT and so signum is irrelevant but adheres
 * to signal() input function requirements.
 */
void
signal_handler (int signum);

/**
 * @brief Given a listening socket file descriptor, returns a connection stream
 * file descriptor and prints client address and port to terminal in numeric
 * form.
 * 
 * @param socket_fd file descriptor for listening socket.
 * @param p_ssl_holder sturcture to fill with ssl and client file descriptors.
 * @return int either the connection stream file descriptor or 
 * FAILURE_NEGATIVE (-1).
 */
int
n_accept (int socket_fd, ssl_socket_holder_t * p_ssl_holder);

/**
 * @brief n_connect uses getaddrinfo to create a list of possible sockaddr
 * structs to attempt to connect to. Once a successful connection is made
 * the connection stream file descriptor is returned.
 * 
 * @param p_address string with the host address in either letter or numeric
 * form.
 * @param p_port string with the port in numeric form.
 * @return int either the value of the file descriptor or FAILURE_NEGATIVE
 * (-1).
 */
int
n_connect (char * p_address, char * p_port);

/**
 * @brief n_bind binds a udp socket on a specified host and port and returns 
 * a socket file descriptor.
 * 
 * @param p_address string with the host address in either letter or numeric
 * form.
 * @param p_port string with the port in numeric form.
 * @return int either the value of the file descriptor or FAILURE_NEGATIVE
 * (-1).
 */
int
n_bind (char * p_address, char * p_port);

/**
 * @brief n_recv_from takes a bound socket file descriptor and writes
 * the recieved data to a provided buffer. It writes the client sockaddr
 * and addrlen to provided adresses to enable to user to utilize the sendto
 * function following the recvfrom utilization.
 * 
 * @param bind_fd bound socket file descriptor.
 * @param p_buffer user-provided buffer pointer.
 * @param buffer_size user-provided buffer size.
 * @param p_temp_addr pointer to sockaddr structure for client address.
 * @param temp_addrlen pointer to addrlen value for client address size.
 * @return int either the number of read bytes or FAILURE_NEGATIVE (-1).
 */
int
n_recv_from (int bind_fd, char * p_buffer, int buffer_size,
    struct sockaddr * p_temp_storage, uint32_t * temp_addrlen);

/**
 * @brief Given an address, port, buffer and buffer size, sends data to
 * a listening udp socket.
 * 
 * @param p_address string with the host address in either letter or numeric
 * form.
 * @param p_port string with the port in numeric form.
 * @param p_buffer user-provided buffer pointer.
 * @param buffer_size user-provided buffer size.
 * @return int either the number of bytes sent or FAILURE_NEGATIVE (-1).
 */
int
n_send_to_socket (char * p_address, char * p_port, char * p_buffer,
                                                  int buffer_size);

/**
 * @brief Receives n bytes from a socket and writes them to the supplied 
 * buffer. 
 * Libraries required: errno.h, sys/socket.h.
 * 
 * @param fd socket file descriptor.
 * @param p_buffer pointer to buffer to write to.
 * @param count number of bytes to recv.
 * @param flags flags argument to use with recv.
 * @return ssize_t Either the number of bytes read, 0 if EOF on first read,
 * or FAILURE_NEGATIVE (1).
 * Note: ssize_t enables negative error codes.
 */
ssize_t
recv_n (int fd, void * p_buffer, size_t count, int flags);

/**
 * @brief Sends n bytes to a socket and writes them to the supplied 
 * buffer. 
 * Libraries required: errno.h, sys/socket.h.
 * 
 * @param fd socket file descriptor.
 * @param p_buffer pointer to buffer to write from.
 * @param count number of bytes to send.
 * @param flags flags argument to use with send.
 * @return ssize_t Either the number of bytes sent, or FAILURE_NEGATIVE (1).
 * Note: ssize_t enables negative error codes.
 */
ssize_t
send_n (int fd, void * p_buffer, size_t count, int flags);

#endif //NETWORKING_LIB

//End of networking.h file
