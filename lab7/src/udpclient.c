#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *host = NULL;
    const char *message = NULL;
    int port = 0;
    int timeout_sec = 2;

    static struct option options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"message", required_argument, 0, 'm'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "h:p:m:t:", options, NULL)) != -1) {
        switch (c) {
        case 'h': host = optarg; break;
        case 'p': port = atoi(optarg); break;
        case 'm': message = optarg; break;
        case 't': timeout_sec = atoi(optarg); break;
        default: return 1;
        }
    }

    if (host == NULL || message == NULL ||
        port <= 0 || port > 65535 || timeout_sec <= 0) {
        printf("Usage: %s --host IP --port PORT --message TEXT [--timeout SEC]\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct timeval timeout = { .tv_sec = timeout_sec, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &address.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address\n");
        close(fd);
        return 1;
    }

    if (sendto(fd, message, strlen(message), 0,
               (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("sendto");
        close(fd);
        return 1;
    }

    char buffer[1024];
    ssize_t n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            printf("No UDP reply before timeout\n");
        else
            perror("recvfrom");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    printf("UDP reply: %s\n", buffer);

    close(fd);
    return 0;
}
