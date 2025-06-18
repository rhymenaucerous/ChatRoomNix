#include "networking.h"

//Variable designated to handle SIGINT signal and enable graceful shutdown
//of waiting sockets.
volatile sig_atomic_t server_interrupt = CONTINUE;

/**
 * @brief Helper function that prints standard library function errors.
 *
 * @param p_function String designating in which networking library function
 * the error occured.
 * @param p_function_ran String designating in which standard library function
 * the error occured.
 * @param err_tracker int designating if an error occured or not.
 */
static void
n_error_print (char * p_function, char * p_function_ran, int err_tracker)
{
    if (FAILURE_NEGATIVE == err_tracker)
    {
        fprintf(stderr, "%s: %s", p_function, p_function_ran);
        perror(":");
    }
}

/**
 * @brief Runs functions to load in the SSL library.
 *
 */
void
n_cr_start_ssl ()
{
    SSL_load_error_strings();
    SSL_library_init();
}

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
n_listen (char * p_address, char * p_port, int backlog)
{
    if ((NULL == p_address) || (NULL == p_port))
    {
        fprintf(stderr, "n_listen: input NULL");
        return FAILURE;
    }

    struct addrinfo hints;
    struct addrinfo * p_result;
    struct addrinfo * p_temp_result;
    int socket_fd = -1;
    int err_tracker = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;

    int status = getaddrinfo(p_address, p_port, &hints, &p_result);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
        return FAILURE;
    }

    int optval = 1;

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    for (p_temp_result = p_result; p_temp_result != NULL;
                           p_temp_result = p_temp_result->ai_next)
    {
        socket_fd = socket(p_temp_result->ai_family,
            p_temp_result->ai_socktype, p_temp_result->ai_protocol);

        if (FAILURE_NEGATIVE == socket_fd)
        {
            continue;
        }

        err_tracker = 0;

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                                              &optval, sizeof(int));
        n_error_print("n_listen", "setsockopt SO_REUSEADDR", err_tracker);

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                                 &timeout, sizeof(struct timeval));
        n_error_print("n_listen", "setsockopt SO_RCVTIMEO", err_tracker);

        err_tracker = bind(socket_fd, p_temp_result->ai_addr,
                                   p_temp_result->ai_addrlen);
        n_error_print("n_listen", "bind", err_tracker);

        err_tracker = listen(socket_fd, backlog);

        if (FAILURE_NEGATIVE == err_tracker)
        {
            perror("n_listen: listen() failure");
            continue;
        }
        else
        {
            break;
        }
    }

    freeaddrinfo(p_result);

    if ((NULL == p_temp_result) || (err_tracker > 0))
    {
        fprintf(stderr, "Listen socket creation failure\n");
        return FAILURE_NEGATIVE;
    }
    else
    {
        return socket_fd;
    }

}

/**
 * @brief Handles SIGINT (cntrl+C) commands by user to set external variable
 * server_interrupt and allow graceful shutdown for the server.
 *
 * @param signum Value of signal being recieved. Signal() usage in library
 * dictates this will only be SIGINT and so signum is irrelevant but adheres
 * to signal() input function requirements.
 */
void
signal_handler (int signum)
{
    if (SIGINT == signum)
    {
        server_interrupt = STOP;
    }
}

/**
 * @brief Creates an ssl context using the TLS server method and server .crt
 * and .key files. For this client's implementation, these will be self signed
 * certificates and not verified by the client.
 *
 * @return SSL_CTX * returns an ssl context to be used with the accepted
 * client socket.
 */
SSL_CTX *
createSSLContext() {
    SSL_CTX * p_ctx = SSL_CTX_new(TLS_server_method());
    if (NULL == p_ctx) {
        ERR_print_errors_fp(stderr); //NOTE: SSL specific error reporting.
        return NULL;
    }

    if (SSL_CTX_use_certificate_file(p_ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(p_ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    return p_ctx;
}

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
n_accept (int socket_fd, ssl_socket_holder_t * p_ssl_holder)
{
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_storage client_addr;
    uint32_t addrlen = sizeof(struct sockaddr_storage);

    char p_host[HOST_MAX_STRING] = {0};
    char p_port[PORT_MAX_STRING] = {0};
    char addr_str[ADDR_MAX_STRING] = {0};

    int flags = NI_NUMERICSERV | NI_NUMERICHOST;

    int client_fd = 0;

    while (CONTINUE == server_interrupt)
    {
        client_fd = accept(socket_fd, (struct sockaddr *)(&client_addr),
                                                              &addrlen);
        if (FAILURE_NEGATIVE == client_fd)
        {
            //The SO_RCVTIMEO flag has been set on the server socket. This
            //means that the accept will fail every 3 seconds. If the
            //following errors are recieved it means that timeout has been
            //met. Unless there is a different error, we want the server to
            //continue to run.
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                                       (errno == EINPROGRESS))
            {
                continue;
            }
            else
            {
                perror("n_accept: accept() failure");
                return FAILURE_NEGATIVE;
            }
        }
        else
        {
            //NOTE: This ssl context must be freed whenever the client socket is closed.
            SSL_CTX * p_ssl_ctx = createSSLContext();

            if (NULL == p_ssl_ctx)
            {
                fprintf(stderr, "n_accept: createSSLContext\n");
                return FAILURE_NEGATIVE;
            }

            SSL * p_ssl = SSL_new(p_ssl_ctx);
            if (NULL == p_ssl)
            {
                fprintf(stderr, "n_accept: SSL_new\n");
                return FAILURE_NEGATIVE;
            }

            SSL_set_fd(p_ssl, client_fd);
            if (0 == p_ssl)
            {
                fprintf(stderr, "n_accept: SSL_set_fd\n");
                return FAILURE_NEGATIVE;
            }

            p_ssl_holder->p_ssl = p_ssl;
            p_ssl_holder->client_fd = client_fd;
            p_ssl_holder->p_ssl_ctx = p_ssl_ctx;

            if (0 >= SSL_accept(p_ssl))
            {
                fprintf(stderr, "SSL_accept: client connection failure\n");
                SSL_free(p_ssl_holder->p_ssl);
                SSL_CTX_free(p_ssl_holder->p_ssl_ctx);
                continue;
            }
            else
            {
                break;
            }
        }
    }

    if (CONTINUE != server_interrupt)
    {
        return FAILURE_NEGATIVE;
    }

    int status = getnameinfo((struct sockaddr *)&client_addr, addrlen,
             p_host, HOST_MAX_STRING, p_port, PORT_MAX_STRING, flags);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
    }
    else
    {
        snprintf(addr_str, ADDR_MAX_STRING, "%s: %s", p_host, p_port);
        printf("Connection recieved from: %s\n", addr_str);
    }

    return client_fd;
}

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
n_connect (char * p_address, char * p_port)
{
    if ((NULL == p_address) || (NULL == p_port))
    {
        fprintf(stderr, "n_connect: input NULL");
        return FAILURE;
    }

    struct addrinfo hints;
    struct addrinfo * p_result;
    struct addrinfo * p_temp_result;
    int socket_fd = -1;
    int err_tracker = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;

    int status = getaddrinfo(p_address, p_port, &hints, &p_result);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
        return FAILURE;
    }

    for (p_temp_result = p_result; p_temp_result != NULL;
                           p_temp_result = p_temp_result->ai_next)
    {
        socket_fd = socket(p_temp_result->ai_family,
            p_temp_result->ai_socktype, p_temp_result->ai_protocol);

        if (FAILURE_NEGATIVE == socket_fd)
        {
            continue;
        }

        err_tracker = 0;

        err_tracker = connect(socket_fd, p_temp_result->ai_addr,
                                      p_temp_result->ai_addrlen);

        if (FAILURE_NEGATIVE == err_tracker)
        {
            perror("n_connect: connect() failure");
            continue;
        }
        else
        {
            break;
        }
    }

    freeaddrinfo(p_result);

    if ((NULL == p_temp_result) || (err_tracker > 0))
    {
        fprintf(stderr, "Connection failure\n");
        return FAILURE_NEGATIVE;
    }
    else
    {
        return socket_fd;
    }
}

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
n_bind (char * p_address, char * p_port)
{
   if ((NULL == p_address) || (NULL == p_port))
    {
        fprintf(stderr, "n_listen: input NULL");
        return FAILURE;
    }

    struct addrinfo hints;
    struct addrinfo * p_result;
    struct addrinfo * p_temp_result;
    int socket_fd = -1;
    int err_tracker = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;

    int status = getaddrinfo(p_address, p_port, &hints, &p_result);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
        return FAILURE;
    }

    int optval = 1;

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    for (p_temp_result = p_result; p_temp_result != NULL;
                           p_temp_result = p_temp_result->ai_next)
    {
        socket_fd = socket(p_temp_result->ai_family,
            p_temp_result->ai_socktype, p_temp_result->ai_protocol);

        if (FAILURE_NEGATIVE == socket_fd)
        {
            continue;
        }

        err_tracker = 0;

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                                              &optval, sizeof(int));
        n_error_print("n_listen", "setsockopt SO_REUSEADDR", err_tracker);

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,
                                 &timeout, sizeof(struct timeval));
        n_error_print("n_listen", "setsockopt SO_RCVTIMEO", err_tracker);

        err_tracker = bind(socket_fd, p_temp_result->ai_addr,
                                   p_temp_result->ai_addrlen);

        if (FAILURE_NEGATIVE == err_tracker)
        {
            perror("n_bind: bind() failure");
            continue;
        }
        else
        {
            break;
        }
    }

    freeaddrinfo(p_result);

    if ((NULL == p_temp_result) || (err_tracker > 0))
    {
        fprintf(stderr, "Bind socket creation failure\n");
        return FAILURE_NEGATIVE;
    }
    else
    {
        return socket_fd;
    }
}

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
    struct sockaddr * p_temp_addr, uint32_t * temp_addrlen)
{
    if (NULL == p_buffer)
    {
        fprintf(stderr, "n_recv_from: input buffer NULL.\n");
        return FAILURE_NEGATIVE;
    }

    struct sockaddr_storage client_addr;
    uint32_t addrlen = sizeof(struct sockaddr_storage);

    char p_host[HOST_MAX_STRING] = {0};
    char p_port[PORT_MAX_STRING] = {0};
    char addr_str[ADDR_MAX_STRING] = {0};

    int flags = NI_NUMERICSERV | NI_NUMERICHOST;

    ssize_t read_bytes = 0;

    while (CONTINUE == server_interrupt)
    {
        read_bytes = recvfrom(bind_fd, p_buffer, buffer_size, MSG_WAITALL,
                             (struct sockaddr *)(&client_addr), &addrlen);
        if (FAILURE_NEGATIVE == read_bytes)
        {
            //See explanation in n_accept above.
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                                         (errno == ETIMEDOUT))
            {
                continue;
            }
            else
            {
                perror("n_recv_from: recv_from() failure");
                return FAILURE_NEGATIVE;
            }
        }
        else
        {
            if ((NULL != p_temp_addr) && (NULL != temp_addrlen))
            {
                memcpy(p_temp_addr, &client_addr, addrlen);
                memcpy(temp_addrlen, &addrlen, sizeof(uint32_t));
            }
            break;
        }
    }

    n_error_print("n_recv_from", "recv_from", read_bytes);

    int status = getnameinfo((struct sockaddr *)&client_addr, addrlen,
             p_host, HOST_MAX_STRING, p_port, PORT_MAX_STRING, flags);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
    }
    else
    {
        snprintf(addr_str, ADDR_MAX_STRING, "%s: %s", p_host, p_port);
        printf("Connection recieved from: %s\n", addr_str);
    }

    return read_bytes;
}

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
                                                   int buffer_size)
{
   if ((NULL == p_address) || (NULL == p_port))
    {
        fprintf(stderr, "n_listen: input NULL");
        return FAILURE;
    }

    struct addrinfo hints;
    struct addrinfo * p_result;
    struct addrinfo * p_temp_result;
    int socket_fd = -1;
    int err_tracker = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;

    int status = getaddrinfo(p_address, p_port, &hints, &p_result);

    if (status != 0)
    {
        fprintf(stderr, "n_listen: getaddrinfo error: %s\n",
                                      gai_strerror(status));
        return FAILURE;
    }

    int optval = 1;

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    for (p_temp_result = p_result; p_temp_result != NULL;
                           p_temp_result = p_temp_result->ai_next)
    {
        socket_fd = socket(p_temp_result->ai_family,
            p_temp_result->ai_socktype, p_temp_result->ai_protocol);

        if (FAILURE_NEGATIVE == socket_fd)
        {
            continue;
        }

        err_tracker = 0;

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                                              &optval, sizeof(int));
        n_error_print("n_send_to_socket", "setsockopt SO_REUSEADDR",
                                                      err_tracker);

        err_tracker = setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO,
                                 &timeout, sizeof(struct timeval));
        n_error_print("n_send_to_socket", "setsockopt SO_SNDTIMEO",
                                                     err_tracker);

        if (FAILURE_NEGATIVE == err_tracker)
        {
            perror("n_bind: bind() failure");
            continue;
        }
        else
        {
            break;
        }
    }

    if ((NULL == p_temp_result) || (err_tracker > 0))
    {
        fprintf(stderr, "socket creation failure\n");
        return FAILURE_NEGATIVE;
    }

    int bytes_sent = sendto(socket_fd, p_buffer, buffer_size, MSG_EOR,
                   p_temp_result->ai_addr, p_temp_result->ai_addrlen);

    if (FAILURE_NEGATIVE == bytes_sent)
    {
        fprintf(stderr, "n_send_to_socket: sendto() failure.\n");
        return FAILURE_NEGATIVE;
    }

    freeaddrinfo(p_result);

    close(socket_fd);

    return bytes_sent;
}

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
recv_n (int fd, void * p_buffer, size_t count, int flags)
{
    ssize_t temp_read_bytes;
    size_t read_bytes;
    char * p_buffer_holder;
    p_buffer_holder = p_buffer;

    for (read_bytes = 0; read_bytes < count; )
    {
        temp_read_bytes = recv(fd, p_buffer_holder, count - read_bytes, flags);

        if (0 == temp_read_bytes) //Received EOF from recv()
        {
            return read_bytes;
        }

        if (FAILURE_NEGATIVE == temp_read_bytes) //recv() failure
        {
            if (EINTR == errno) //interrupted system call, try again
            {
                continue;
            }
            else
            {
                perror("recvn(): recv");
                return FAILURE_NEGATIVE;
            }
        }

        read_bytes += temp_read_bytes;
        p_buffer_holder += temp_read_bytes;
    }

    return read_bytes;
}

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
send_n (int fd, void * p_buffer, size_t count, int flags)
{
    ssize_t temp_sent_bytes;
    size_t sent_bytes;
    const char * p_buffer_holder;
    p_buffer_holder = p_buffer;

    for (sent_bytes = 0; sent_bytes < count; )
    {
        temp_sent_bytes = send(fd, p_buffer_holder, count - sent_bytes, flags);

        if (temp_sent_bytes <= 0) //error
        {
            if ((FAILURE_NEGATIVE == temp_sent_bytes) && ((EINTR == errno)
                          || (EAGAIN == errno) || (EWOULDBLOCK == errno)))
            {
                continue; //send interrupted, try again
            }
            else
            {
                return FAILURE_NEGATIVE; //any other error
            }
        }

        sent_bytes += temp_sent_bytes;
        p_buffer_holder += temp_sent_bytes;
    }

    return sent_bytes;
}

//End of networking.c file
