#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf(
            "Child PID: %d\n",
            getpid()
        );

        return 0;
    }

    printf(
        "Parent PID: %d\n",
        getpid()
    );

    printf(
        "Child PID: %d\n",
        pid
    );

    printf(
        "For 30 seconds child is a zombie.\n"
    );

    sleep(30);

    wait(NULL);

    printf(
        "Zombie removed by wait().\n"
    );

    return 0;
}
