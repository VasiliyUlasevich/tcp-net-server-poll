//
// Created by trac on 11/19/24.
//

#ifndef C11_TCP_SERVER_H
#define C11_TCP_SERVER_H

#include <signal.h>
#include <stdint.h>

#define MAX_SIGNAL_NUM    64
#define FDS_INC_STEP   1024

typedef void* (*connection_handler_p)(void* client_socket);

static volatile sig_atomic_t http_abort_signal = 0;

void sig_handler(int sig_no);
void set_signal_handler();
int init_listen_server(char *address, uint16_t port);
void start_server_loop(int server_socket, connection_handler_p handler);

#endif //C11_TCP_SERVER_H
