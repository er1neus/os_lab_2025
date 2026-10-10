#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct ThreadArgs {
    unsigned long long begin;
    unsigned long long end;
    unsigned long long mod;
};

static unsigned long long result = 1;
static pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    unsigned long long local = 1 % args->mod;

    for (unsigned long long i = args->begin; i <= args->end; i++)
        local = (local * (i % args->mod)) % args->mod;

    pthread_mutex_lock(&result_mutex);
    result = (result * local) % args->mod;
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

int main(int argc, char **argv)
{
    unsigned long long k = 0;
    unsigned long long mod = 0;
    int pnum = 0;

    static struct option options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "k:", options, NULL)) != -1) {
        switch (c) {
        case 'k':
            k = strtoull(optarg, NULL, 10);
            break;
        case 'p':
            pnum = atoi(optarg);
            break;
        case 'm':
            mod = strtoull(optarg, NULL, 10);
            break;
        default:
            return 1;
        }
    }

    if (k == 0 || pnum <= 0 || mod == 0) {
        printf("Usage: %s -k N --pnum=N --mod=N\n", argv[0]);
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
    struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs) * pnum);

    if (threads == NULL || args == NULL) {
        perror("malloc");
        free(threads);
        free(args);
        return 1;
    }

    for (int i = 0; i < pnum; i++) {
        args[i].begin = 1 + (k * i) / pnum;
        args[i].end = (k * (i + 1)) / pnum;
        args[i].mod = mod;

        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < pnum; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&result_mutex);

    printf("%llu! %% %llu = %llu\n", k, mod, result);

    free(args);
    free(threads);

    return 0;
}
