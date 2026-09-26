#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_happened = 0;

void AlarmHandler(int sig)
{
    timeout_happened = 1;
}

int main(int argc, char **argv)
{
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0;

    bool with_files = false;

    static struct option options[] = {
        {"seed", required_argument, 0, 's'},
        {"array_size", required_argument, 0, 'a'},
        {"pnum", required_argument, 0, 'p'},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int c;

    while ((c = getopt_long(
                argc,
                argv,
                "s:a:p:ft:",
                options,
                NULL)) != -1)
    {
        switch (c)
        {
            case 's':
                seed = atoi(optarg);
                break;

            case 'a':
                array_size = atoi(optarg);
                break;

            case 'p':
                pnum = atoi(optarg);
                break;

            case 'f':
                with_files = true;
                break;

            case 't':
                timeout = atoi(optarg);
                break;

            default:
                return 1;
        }
    }

    if (seed < 0 ||
        array_size <= 0 ||
        pnum <= 0 ||
        pnum > array_size ||
        timeout < 0)
    {
        printf(
            "Usage: %s "
            "--seed num "
            "--array_size num "
            "--pnum num "
            "[--by_files] "
            "[--timeout num]\n",
            argv[0]
        );

        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);

    GenerateArray(array, array_size, seed);

    pid_t children[pnum];

    int pipefd[2];

    if (!with_files)
        pipe(pipefd);

    struct timeval start_time;

    gettimeofday(&start_time, NULL);

    for (int i = 0; i < pnum; i++)
    {
        pid_t child_pid = fork();

        if (child_pid < 0)
        {
            printf("Fork failed!\n");
            free(array);

            return 1;
        }

        if (child_pid == 0)
        {
            unsigned int begin =
                i * array_size / pnum;

            unsigned int end =
                (i + 1) * array_size / pnum;

            struct MinMax part =
                GetMinMax(array, begin, end);

            if (with_files)
            {
                char filename[32];

                sprintf(
                    filename,
                    "minmax_%d.tmp",
                    i
                );

                FILE *file =
                    fopen(filename, "w");

                fprintf(
                    file,
                    "%d %d\n",
                    part.min,
                    part.max
                );

                fclose(file);
            }
            else
            {
                close(pipefd[0]);

                write(
                    pipefd[1],
                    &part,
                    sizeof(part)
                );

                close(pipefd[1]);
            }

            free(array);

            return 0;
        }

        children[i] = child_pid;
    }

    if (!with_files)
        close(pipefd[1]);

    /*
     * Если timeout задан, используем alarm()
     * и неблокирующий waitpid().
     */
    if (timeout > 0)
    {
        signal(SIGALRM, AlarmHandler);

        alarm(timeout);

        int finished = 0;

        while (finished < pnum &&
               !timeout_happened)
        {
            pid_t pid =
                waitpid(-1, NULL, WNOHANG);

            if (pid > 0)
            {
                finished++;
            }
            else if (pid == 0)
            {
                usleep(10000);
            }
            else if (errno != EINTR)
            {
                break;
            }
        }

        if (timeout_happened)
        {
            for (int i = 0; i < pnum; i++)
            {
                kill(
                    children[i],
                    SIGKILL
                );
            }

            /*
             * Забираем завершившихся детей,
             * чтобы не оставлять zombie.
             */
            while (wait(NULL) > 0)
            {
            }

            printf("Timeout!\n");

            free(array);

            return 1;
        }

        alarm(0);
    }

    /*
     * Если timeout не задан,
     * сохраняем обычное поведение.
     */
    else
    {
        for (int i = 0; i < pnum; i++)
        {
            wait(NULL);
        }
    }

    struct MinMax min_max;

    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++)
    {
        struct MinMax part;

        if (with_files)
        {
            char filename[32];

            sprintf(
                filename,
                "minmax_%d.tmp",
                i
            );

            FILE *file =
                fopen(filename, "r");

            fscanf(
                file,
                "%d %d",
                &part.min,
                &part.max
            );

            fclose(file);

            remove(filename);
        }
        else
        {
            read(
                pipefd[0],
                &part,
                sizeof(part)
            );
        }

        if (part.min < min_max.min)
            min_max.min = part.min;

        if (part.max > min_max.max)
            min_max.max = part.max;
    }

    if (!with_files)
        close(pipefd[0]);

    struct timeval finish_time;

    gettimeofday(
        &finish_time,
        NULL
    );

    double elapsed_time =
        (finish_time.tv_sec -
         start_time.tv_sec) * 1000.0;

    elapsed_time +=
        (finish_time.tv_usec -
         start_time.tv_usec) / 1000.0;

    free(array);

    printf(
        "Min: %d\n",
        min_max.min
    );

    printf(
        "Max: %d\n",
        min_max.max
    );

    printf(
        "Elapsed time: %fms\n",
        elapsed_time
    );

    return 0;
}