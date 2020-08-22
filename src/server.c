#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>

#define server_max_clients 32

int server_listen_socket_fd;
Client server_conn_clients[server_max_clients];

/**
 * Send welcome message to client
 *
 * @param client_id
 */
void client_send_welcome(int client_id);

/**
 * Error handling wrapper for socket functions
 * Stolen from StackOverflow
 *
 * @param n
 * @param err
 * @return
 */
int server_guard(int n, char *err) {
    if (n == -1) {
        perror(err);
        exit(1);
    }
    return n;
}

/**
 * Open TCP socket with O_NONBLOCK flag
 *
 * @param port
 */
void server_start(int port) {
    bzero(server_conn_clients, sizeof(server_conn_clients));
    server_listen_socket_fd = server_guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
    int flags = server_guard(fcntl(server_listen_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
    server_guard(fcntl(server_listen_socket_fd, F_SETFL, flags | O_NONBLOCK),
                 "could not set TCP listening socket to be non-blocking");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_guard(bind(server_listen_socket_fd, (struct sockaddr *) &addr, sizeof(addr)), "could not bind");
    server_guard(listen(server_listen_socket_fd, 100), "could not listen");
    printf("Listening on %d\n", port);
}

/**
 * Disconnect a client
 *
 * @param client_id
 */
void server_client_disconnect(int client_id) {
    server_conn_clients[client_id].disconnect = 1;
    close(server_conn_clients[client_id].fd);
    printf("Client #%d disconnected\n", client_id);
}

/**
 * Send a message to a client, disconnect client on failure
 *
 * @param client_id
 * @param message
 * @param length
 * @return
 */
int server_send_client(int client_id, char *message, size_t length) {
    int res = send(server_conn_clients[client_id].fd, message, length, MSG_NOSIGNAL);
    if (res == -1) {
        server_client_disconnect(client_id);
    }
    return res;
}

/**
 * Read data from client, disconnect on failure, ignore EWOULDBLOCK
 *
 * @param client_id
 * @param buf
 * @param length
 * @return
 */
int server_read_client(int client_id, void *buf, size_t length) {
    int res = read(server_conn_clients[client_id].fd, buf, length);
    if (res == -1) {
        if (errno != EWOULDBLOCK) {
            server_client_disconnect(client_id);
        }
    } else {
        server_conn_clients[client_id].last_update = time(NULL);
    }
    return res;
}

/**
 * Check if a a client exists and is active
 *
 * @param client_id
 * @return
 */
int server_client_active(int client_id) {
    return server_conn_clients[client_id].fd && !server_conn_clients[client_id].disconnect;
}

/**
 * Initialize a client from a file descriptor
 *
 * @param client_socket_fd
 */
void server_init_client(int client_socket_fd) {
    int flags = server_guard(fcntl(client_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
    server_guard(fcntl(client_socket_fd, F_SETFL, flags | O_NONBLOCK),
                 "could not set TCP listening socket to be non-blocking");

    int free_id = -1;
    for (int i = 0; i < server_max_clients; i++) {
        if (!server_client_active(i)) {
            free_id = i;
            break;
        }
    }
    if (free_id == -1) {
        char msg[] = "Connection limit reached\r\n";
        printf("Failed to accept new connection: Too many clients already connected\n");
        send(client_socket_fd, msg, sizeof(msg), 0);
        close(client_socket_fd);
        return;
    }

    server_conn_clients[free_id] = (Client) {
            free_id,
            client_socket_fd,
            0,
            0,
            time(NULL),
            {0, 0}
    };

    printf("Client #%d connected\n", free_id);
    client_send_welcome(free_id);
}

/**
 * Read a single byte from a client
 *
 * @param client_id
 * @return
 */
char server_client_read_char(int client_id) {
    char oneByte = 0;
    server_read_client(client_id, &oneByte, 1);
    return oneByte;
}

/**
 * Get the pressed key from a client
 * Basically server_client_read_char, but arrow keys are mapped to WASD in a hacky way
 *
 * @param client_id
 * @return
 */
char server_client_get_key(int client_id) {
    char c = server_client_read_char(client_id);
    if (c != 27) {
        return (char) c;
    }
    if (server_client_read_char(client_id) != 91) {
        return 0;
    }
    switch (server_client_read_char(client_id)) {
        case 65:
            return 'w';
        case 66:
            return 's';
        case 67:
            return 'd';
        case 68:
            return 'a';
        default:
            return 0;
    }
}

/**
 * Get the pointer to a Client from a client id
 *
 * @param client_id
 * @return
 */
Client *server_get_client(int client_id) {
    return &server_conn_clients[client_id];
}

/**
 * Accept and initialize a new client
 *
 * @return
 */
int server_accept_client() {
    int client_socket_fd = accept(server_listen_socket_fd, NULL, NULL);
    if (client_socket_fd == -1) {
        if (errno != EWOULDBLOCK) {
            perror("error when accepting connection");
        }
        return 0;
    }
    server_init_client(client_socket_fd);
    return 1;
}

/**
 * Update a client
 * Currently only used to disconnect inactive clients after three minutes
 *
 * @param cid
 * @return
 */
int server_update_client(int cid) {
    if (server_conn_clients[cid].last_update + 60 * 3 < time(NULL)) {
        printf("Client #%d timed out\n", cid);

        char msg[] = "\033[0mConnection timed out\r\n";
        server_send_client(cid, msg, strlen(msg));
        server_client_disconnect(cid);
        return 0;
    }
    return 1;
}

/**
 * Close the TCP socket
 *
 */
void server_close() {
    for (int cid = 0; cid < server_max_clients; cid++) {
        if (!server_client_active(cid)) {
            continue;
        }
        server_client_disconnect(cid);
    }
    close(server_listen_socket_fd);
    printf("Server closed\n");
}

