#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            break;

          case 1:
            array_size = atoi(optarg);
            break;

          case 2:
            pnum = atoi(optarg);
            break;

          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;

      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed <= 0 || array_size <= 0 || pnum <= 0 || pnum > array_size) {
    printf(
        "Usage: %s --seed \"num\" --array_size \"num\" "
        "--pnum \"num\" [--by_files]\n",
        argv[0]
    );
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);

  if (array == NULL) {
    perror("malloc");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  /*
   * Если работаем не через файлы, создаём отдельный pipe
   * для каждого дочернего процесса.
   *
   * pipes[i][0] - чтение
   * pipes[i][1] - запись
   */
  int (*pipes)[2] = NULL;

  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);

    if (pipes == NULL) {
      perror("malloc");
      free(array);
      return 1;
    }

    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        free(pipes);
        free(array);
        return 1;
      }
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  /*
   * Создаём pnum дочерних процессов.
   */
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
      perror("fork");
      free(pipes);
      free(array);
      return 1;
    }

    /*
     * Код дочернего процесса.
     */
    if (child_pid == 0) {
      /*
       * Делим массив между процессами.
       *
       * Например:
       * array_size = 10
       * pnum = 3
       *
       * child 0: [0, 3)
       * child 1: [3, 6)
       * child 2: [6, 10)
       */
      unsigned int begin = i * array_size / pnum;
      unsigned int end = (i + 1) * array_size / pnum;

      struct MinMax part = GetMinMax(array, begin, end);

      /*
       * Вариант через файлы.
       */
      if (with_files) {
        char filename[64];

        snprintf(
            filename,
            sizeof(filename),
            "minmax_%d.tmp",
            i
        );

        FILE *file = fopen(filename, "w");

        if (file == NULL) {
          perror("fopen");
          free(array);
          _exit(1);
        }

        fprintf(
            file,
            "%d %d\n",
            part.min,
            part.max
        );

        fclose(file);
      }

      /*
       * Вариант через pipe.
       */
      else {
        /*
         * Ребёнку не нужны read-концы pipe.
         *
         * Также ему нужен write-конец только собственного pipe.
         */
        for (int j = 0; j < pnum; j++) {
          close(pipes[j][0]);

          if (j != i)
            close(pipes[j][1]);
        }

        if (write(
                pipes[i][1],
                &part,
                sizeof(part)
            ) != sizeof(part)) {

          perror("write");

          close(pipes[i][1]);

          free(pipes);
          free(array);

          _exit(1);
        }

        close(pipes[i][1]);
      }

      free(pipes);
      free(array);

      _exit(0);
    }

    /*
     * Эту часть выполняет родитель.
     */
    active_child_processes += 1;
  }

  /*
   * Родитель ничего не пишет в pipe,
   * поэтому закрывает все write-концы.
   */
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  /*
   * Ждём завершения всех дочерних процессов.
   */
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  /*
   * Теперь собираем результаты.
   */
  struct MinMax min_max;

  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    /*
     * Читаем результат ребёнка из файла.
     */
    if (with_files) {
      char filename[64];

      snprintf(
          filename,
          sizeof(filename),
          "minmax_%d.tmp",
          i
      );

      FILE *file = fopen(filename, "r");

      if (file == NULL) {
        perror("fopen");

        free(pipes);
        free(array);

        return 1;
      }

      if (fscanf(file, "%d %d", &min, &max) != 2) {
        fprintf(
            stderr,
            "Failed to read %s\n",
            filename
        );

        fclose(file);

        free(pipes);
        free(array);

        return 1;
      }

      fclose(file);

      /*
       * Временный файл больше не нужен.
       */
      remove(filename);
    }

    /*
     * Читаем структуру MinMax из pipe.
     */
    else {
      struct MinMax part;

      if (read(
              pipes[i][0],
              &part,
              sizeof(part)
          ) != sizeof(part)) {

        perror("read");

        close(pipes[i][0]);

        free(pipes);
        free(array);

        return 1;
      }

      close(pipes[i][0]);

      min = part.min;
      max = part.max;
    }

    /*
     * Объединяем локальные результаты.
     */
    if (min < min_max.min)
      min_max.min = min;

    if (max > min_max.max)
      min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time =
      (finish_time.tv_sec - start_time.tv_sec) * 1000.0;

  elapsed_time +=
      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(pipes);
  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  fflush(NULL);

  return 0;
}