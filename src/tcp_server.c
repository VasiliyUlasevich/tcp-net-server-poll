//
// Created by trac on 11/19/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "tcp_server.h"

void sig_handler(int sig_no) {
    if (sig_no == SIGINT) {
        fprintf(stderr, "SIGNAL: Received SIGINT\n");
        http_abort_signal = 1;
    } else {
        // another signal (SIGUSR1)
        // for test purposes
    }
}

void set_signal_handler() {
    struct sigaction sa;

    memset(&sa, 0 , sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    for (int i = 1; i < MAX_SIGNAL_NUM + 1; ++i) {
        if (i != SIGINT && i != SIGUSR1 && i != SIGKILL && i != SIGSTOP && i != 32 && i != 33) {
            if (sigaction(i, &sa, NULL) == -1) {
                fprintf(stderr, "ERROR: Cannot assign signal(%d) handler\n", i);
                perror("sigaction.");
            }
        }
    }
    sa.sa_handler = &sig_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "ERROR: Cannot assign SIGINT handler\n");
        perror("sigaction.");
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "ERROR: Cannot assign SIGUSR1 handler\n");
        perror("sigaction.");
    }
}

int init_listen_server(char *address, uint16_t port) {
    int s;
    struct sockaddr_in socket_address;
    int true_value = 1;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        fprintf(stderr,"ERROR: socket()\n");
        return 0;
    }

    if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&true_value,sizeof(true_value)) == -1)
    {
        fprintf(stderr,"ERROR: setsockopt()\n");
        return 0;
    }

    // set as nonblocking
    if (ioctl(s, FIONBIO, (char *)&true_value) == -1) {
        fprintf(stderr,"ERROR: ioctl()\n");
        return 0;
    }

    memset((void *)&socket_address, 0, sizeof(socket_address));
    socket_address.sin_addr.s_addr = inet_addr(address);

    if (!inet_aton(address, &socket_address.sin_addr)) {
        fprintf(stderr,"ERROR: inet_addr()\n");
        errno = EFAULT;
        return 0;
    }

    socket_address.sin_port = htons(port);
    socket_address.sin_family = AF_INET;

    if (bind(s, (struct sockaddr *)&socket_address, sizeof(socket_address)) == -1) {
        fprintf(stderr,"ERROR: bind()\n");
        return 0;
    }

    if (listen(s,SOMAXCONN) == -1) {
        fprintf(stderr,"ERROR: listen()\n");
        return 0;
    }

    // return server socket
    return s;
}

void start_server_loop(int server_socket, connection_handler_p handler) {
    nfds_t opened_fds, reserved_fds = FDS_INC_STEP;
    struct pollfd *pfds, *tmp_pfds;
    int rc;
    int new_socket;

    pfds = calloc(reserved_fds, sizeof(struct pollfd)); // reserve space for 1000 items

    opened_fds = 1;

    pfds[0].fd = server_socket;
    pfds[0].events = POLLIN ;

    while(!http_abort_signal) {
        rc = poll(pfds, opened_fds, 1);
        if (rc < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "ERROR: poll()\n");
                perror("poll() failed");
                break;
            }
        } else if (rc == 0) {
            // timeout (if timeout value in poll call is greater or eq 0)
            continue;
        } else {
            for (nfds_t i = 0; i < opened_fds; ++i) {
                if (pfds[i].revents !=0) {
                    if (pfds[i].revents & POLLIN) {
                        if (pfds[i].fd == server_socket) {
                            do {
                                new_socket = accept(server_socket, NULL, NULL);
                                if (new_socket != -1) {
                                    if (opened_fds + 1 >= reserved_fds) {
                                        tmp_pfds = realloc(pfds, sizeof(struct pollfd) * (reserved_fds + FDS_INC_STEP));
                                        if (tmp_pfds == NULL) {
                                            fprintf(stderr,"No memory for incoming connection\n");
                                            close(new_socket);
                                            break;
                                        } else {
                                            reserved_fds += FDS_INC_STEP;
                                            pfds = tmp_pfds;
                                            fprintf(stdout, "reallocate memory to %lu connections\n", reserved_fds);
                                        }
                                    }
                                    pfds[opened_fds].fd = new_socket;
                                    pfds[opened_fds].events = POLLIN;
                                    ++opened_fds;
                                } else {
                                    if (errno != EWOULDBLOCK) {
                                        fprintf(stderr,"ERROR: accept()\n");
                                        perror("accept()");
                                        break;
                                    }
                                }
                            } while (new_socket != -1);
                        } else {
                            // do useful work for socket
                            handler(&pfds[i].fd);
                            if (i < opened_fds - 1) {
                                pfds[i] = pfds[opened_fds - 1];
                            }
                            --opened_fds;
                            --i;
                            continue;
                        }
                    } else {
                        /* POLLERR | POLLHUP */
                        if (pfds[i].fd == server_socket) {
                            http_abort_signal = 1;
                            break;
                        } else {
                            fprintf(stdout, "POLLERR | POLLUP event\n");
                            close(pfds[i].fd);
                            if (i < opened_fds - 1) {
                                pfds[i] = pfds[opened_fds - 1];
                            }
                            --opened_fds;
                        }
                    }
                }
            }
        }
    }

    free(pfds);
    close(server_socket);
}
