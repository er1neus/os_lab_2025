#include "netlib.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static unsigned long long range_product(
    unsigned long long begin,
    unsigned long long end,
    unsigned long long mod)
{
    unsigned long long result = 1 % mod;

    if (begin > end)
        return result;

    for (unsigned long long i = begin; i <= end; i++)
        result = (result * (i % mod)) % mod;

    return result;
}

int main(int argc, char **argv)
{
    int port = 0;

    static struct option options[] = {
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "p:", options, NULL)) != -1) {
        if (c == 'p')
            port = atoi(optarg);
        else
            return 1;
    }

    if (port <= 0 || port > 65535) {
        printf("Usage: %s --port PORT\n", argv[0]);
        return 1;
    }

    int listen_fd = create_listen_socket(port);
    if (listen_fd < 0) {
        perror("create_listen_socket");
        return 1;
    }

    printf("Server listens on port %d\n", port);
    fflush(stdout);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char request[256];

        if (recv_line(client_fd, request, sizeof(request)) < 0) {
            close(client_fd);
            continue;
        }

        unsigned long long begin;
        unsigned long long end;
        unsigned long long mod;

        if (sscanf(request, "%llu %llu %llu", &begin, &end, &mod) != 3 ||
            mod == 0) {
            const char *error = "ERROR\n";
            send_all(client_fd, error, 6);
            close(client_fd);
            continue;
        }

        unsigned long long result =
            range_product(begin, end, mod);

        char response[128];
        int length = snprintf(
            response,
            sizeof(response),
            "%llu\n",
            result
        );

        send_all(client_fd, response, (size_t)length);
        close(client_fd);
    }

    close(listen_fd);
    return 0;
}
