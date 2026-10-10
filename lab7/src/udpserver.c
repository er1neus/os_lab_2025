#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int port = 0;

    static struct option options[] = {
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "p:", options, NULL)) != -1) {
        if (c == 'p') port = atoi(optarg);
        else return 1;
    }

    if (port <= 0 || port > 65535) {
        printf("Usage: %s --port PORT\n", argv[0]);
        return 1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(fd);
        return 1;
    }

    printf("UDP server listens on port %d\n", port);

    while (1) {
        char buffer[1024];
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        ssize_t n = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&client, &client_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';
        printf("UDP received: %s\n", buffer);

        sendto(fd, buffer, (size_t)n, 0,
               (struct sockaddr *)&client, client_len);
    }

    close(fd);
    return 0;
}
