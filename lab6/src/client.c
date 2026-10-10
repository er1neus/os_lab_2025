#include "netlib.h"

#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct ServerInfo {
    char host[128];
    int port;
};

struct WorkerArgs {
    struct ServerInfo server;

    unsigned long long begin;
    unsigned long long end;
    unsigned long long mod;

    unsigned long long result;

    int error;
};

static int read_servers(
    const char *path,
    struct ServerInfo **servers_out)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
        return -1;

    struct ServerInfo *servers = NULL;
    int count = 0;

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        char host[128];
        int port;

        if (line[0] == '#' || line[0] == '\n')
            continue;

        if (sscanf(
                line,
                "%127[^:]:%d",
                host,
                &port) != 2)
        {
            continue;
        }

        struct ServerInfo *new_servers =
            realloc(
                servers,
                sizeof(*servers) * (count + 1)
            );

        if (new_servers == NULL) {
            free(servers);
            fclose(file);

            return -1;
        }

        servers = new_servers;

        strcpy(
            servers[count].host,
            host
        );

        servers[count].port = port;

        count++;
    }

    fclose(file);

    *servers_out = servers;

    return count;
}

static void *server_worker(void *arg)
{
    struct WorkerArgs *task =
        (struct WorkerArgs *)arg;

    int fd = connect_to_server(
        task->server.host,
        task->server.port
    );

    if (fd < 0) {
        task->error = 1;
        return NULL;
    }

    char request[256];

    int length = snprintf(
        request,
        sizeof(request),
        "%llu %llu %llu\n",
        task->begin,
        task->end,
        task->mod
    );

    if (send_all(
            fd,
            request,
            (size_t)length) < 0)
    {
        task->error = 1;

        close(fd);

        return NULL;
    }

    char response[128];

    if (recv_line(
            fd,
            response,
            sizeof(response)) < 0 ||
        sscanf(
            response,
            "%llu",
            &task->result) != 1)
    {
        task->error = 1;
    }

    close(fd);

    return NULL;
}

int main(int argc, char **argv)
{
    unsigned long long k = 0;
    unsigned long long mod = 0;

    const char *servers_path = NULL;

    static struct option options[] = {
        {"k", required_argument, 0, 'k'},
        {"mod", required_argument, 0, 'm'},
        {"servers", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int c;

    while ((c = getopt_long(
                argc,
                argv,
                "k:m:s:",
                options,
                NULL)) != -1)
    {
        switch (c) {
            case 'k':
                k = strtoull(
                    optarg,
                    NULL,
                    10
                );
                break;

            case 'm':
                mod = strtoull(
                    optarg,
                    NULL,
                    10
                );
                break;

            case 's':
                servers_path = optarg;
                break;

            default:
                return 1;
        }
    }

    if (k == 0 ||
        mod == 0 ||
        servers_path == NULL)
    {
        printf(
            "Usage: %s "
            "--k N "
            "--mod N "
            "--servers FILE\n",
            argv[0]
        );

        return 1;
    }

    struct ServerInfo *servers = NULL;

    int server_count =
        read_servers(
            servers_path,
            &servers
        );

    if (server_count <= 0) {
        fprintf(
            stderr,
            "No servers found\n"
        );

        free(servers);

        return 1;
    }

    pthread_t *threads =
        malloc(
            sizeof(*threads) *
            server_count
        );

    struct WorkerArgs *tasks =
        calloc(
            (size_t)server_count,
            sizeof(*tasks)
        );

    if (threads == NULL ||
        tasks == NULL)
    {
        perror("malloc");

        free(tasks);
        free(threads);
        free(servers);

        return 1;
    }

    /*
     * Делим диапазон [1; k]
     * между серверами.
     */
    for (int i = 0;
         i < server_count;
         i++)
    {
        tasks[i].server = servers[i];

        tasks[i].begin =
            1 +
            (k * i) /
            server_count;

        tasks[i].end =
            (k * (i + 1)) /
            server_count;

        tasks[i].mod = mod;

        /*
         * Для каждого сервера создаём
         * отдельный поток, чтобы запросы
         * выполнялись параллельно.
         */
        pthread_create(
            &threads[i],
            NULL,
            server_worker,
            &tasks[i]
        );
    }

    unsigned long long result =
        1 % mod;

    int failed = 0;

    /*
     * Ждём ответы от серверов
     * и объединяем частичные результаты.
     */
    for (int i = 0;
         i < server_count;
         i++)
    {
        pthread_join(
            threads[i],
            NULL
        );

        if (tasks[i].error) {
            fprintf(
                stderr,
                "Server %s:%d failed\n",
                tasks[i].server.host,
                tasks[i].server.port
            );

            failed = 1;

            continue;
        }

        result =
            (result *
             tasks[i].result)
            % mod;

        printf(
            "%s:%d -> "
            "[%llu, %llu], "
            "partial=%llu\n",
            tasks[i].server.host,
            tasks[i].server.port,
            tasks[i].begin,
            tasks[i].end,
            tasks[i].result
        );
    }

    if (!failed) {
        printf(
            "%llu! %% %llu = %llu\n",
            k,
            mod,
            result
        );
    }

    free(tasks);
    free(threads);
    free(servers);

    return failed ? 1 : 0;
}