#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include "tcp_server.h"

char response[] = "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 67\r\n\r\n"
                  "<!DOCTYPE html><html><body><h1>Sample of response</h1><body></html>";

void* client_connection_handler(void* client_socket_p) {
    int client_socket = *(int*)client_socket_p;
    ssize_t sz;
    char inc_buffer[4096];
    ssize_t rd_size;

    memset(inc_buffer, 0 , sizeof (inc_buffer));
    rd_size = recv(client_socket, inc_buffer, sizeof(inc_buffer), 0);

    if (rd_size < 0) {
        fprintf(stderr, "ERROR: recv()\n");
        perror("Data receive error");
    } else if (rd_size > 0) {

        sz = write(client_socket, (void *)response, sizeof(response) - 1);
        if (sz == -1) {
            fprintf(stderr, "ERROR: write()\n");
            perror("Data send error");
        }
    } else {
        fprintf(stderr, "ERROR: connection was closed from client side\n");
    }

    if (close(client_socket) == -1) {
        fprintf(stderr, "ERROR: close()\n");
        perror("Close socket error");
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    uint16_t port;
    int server_socket; // server socket

    if (argc < 3) {
        printf("%s <iface_address> <port>\n", argv[0]);
        return -1;
    }

    set_signal_handler();

    port = atoi(argv[2]);
    if (!port) {
        fprintf(stderr, "ERROR: Wrong port number\n");
        return -1;
    }

    server_socket = init_listen_server(argv[1], port);
    if (!server_socket) {
        perror("Server initialization error.");
        return -1;
    }

    printf("Server started at %s:%d\n",argv[1], port);

    start_server_loop(server_socket, client_connection_handler);

    printf("Server finished.\n");
    return 0;
}
