#ifndef NETLIB_H
#define NETLIB_H

#include <stddef.h>

int connect_to_server(const char *host, int port);
int create_listen_socket(int port);
int send_all(int fd, const void *buffer, size_t length);
int recv_line(int fd, char *buffer, size_t size);

#endif
