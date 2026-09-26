#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include "sum.h"
#include "utils.h"

void *ThreadSum(void *args)
{
    struct SumArgs *sum_args =
        (struct SumArgs *)args;

    sum_args->result =
        Sum(sum_args);

    return NULL;
}

int main(int argc, char **argv)
{
    int threads_num = 0;
    int array_size = 0;
    int seed = 0;

    static struct option options[] = {
        {
            "threads_num",
            required_argument,
            0,
            't'
        },
        {
            "seed",
            required_argument,
            0,
            's'
        },
        {
            "array_size",
            required_argument,
            0,
            'a'
        },
        {0, 0, 0, 0}
    };

    int c;

    while ((c = getopt_long(
                argc,
                argv,
                "t:s:a:",
                options,
                NULL)) != -1)
    {
        switch (c)
        {
            case 't':
                threads_num =
                    atoi(optarg);
                break;

            case 's':
                seed =
                    atoi(optarg);
                break;

            case 'a':
                array_size =
                    atoi(optarg);
                break;

            default:
                return 1;
        }
    }

    if (threads_num <= 0 ||
        array_size <= 0 ||
        threads_num > array_size)
    {
        printf(
            "Usage: %s "
            "--threads_num num "
            "--seed num "
            "--array_size num\n",
            argv[0]
        );

        return 1;
    }

    int *array =
        malloc(
            sizeof(int) *
            array_size
        );

    /*
     * Массив генерируется ДО начала
     * измерения времени.
     */
    GenerateArray(
        array,
        array_size,
        seed
    );

    pthread_t threads[threads_num];

    struct SumArgs args[threads_num];

    struct timeval start_time;

    gettimeofday(
        &start_time,
        NULL
    );

    /*
     * Создание потоков.
     */
    for (int i = 0;
         i < threads_num;
         i++)
    {
        args[i].array = array;

        args[i].begin =
            i *
            array_size /
            threads_num;

        args[i].end =
            (i + 1) *
            array_size /
            threads_num;

        args[i].result = 0;

        if (pthread_create(
                &threads[i],
                NULL,
                ThreadSum,
                &args[i]))
        {
            printf(
                "Error: pthread_create failed!\n"
            );

            free(array);

            return 1;
        }
    }

    long long total_sum = 0;

    /*
     * Ожидаем каждый поток
     * и складываем результаты.
     */
    for (int i = 0;
         i < threads_num;
         i++)
    {
        pthread_join(
            threads[i],
            NULL
        );

        total_sum +=
            args[i].result;
    }

    struct timeval finish_time;

    gettimeofday(
        &finish_time,
        NULL
    );

    double elapsed_time =
        (finish_time.tv_sec -
         start_time.tv_sec) *
        1000.0;

    elapsed_time +=
        (finish_time.tv_usec -
         start_time.tv_usec) /
        1000.0;

    printf(
        "Total: %lld\n",
        total_sum
    );

    printf(
        "Elapsed time: %fms\n",
        elapsed_time
    );

    free(array);

    return 0;
}