#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *host = NULL;
    const char *message = NULL;
    int port = 0;

    static struct option options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"message", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "h:p:m:", options, NULL)) != -1) {
        switch (c) {
        case 'h': host = optarg; break;
        case 'p': port = atoi(optarg); break;
        case 'm': message = optarg; break;
        default: return 1;
        }
    }

    if (host == NULL || message == NULL || port <= 0 || port > 65535) {
        printf("Usage: %s --host IP --port PORT --message TEXT\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address\n");
        close(fd);
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    send(fd, message, strlen(message), 0);

    char buffer[1024];
    ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (n > 0) {
        buffer[n] = '\0';
        printf("TCP reply: %s\n", buffer);
    } else if (n == 0) {
        printf("Server closed connection\n");
    } else {
        perror("recv");
    }

    close(fd);
    return 0;
}
