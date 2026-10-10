#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t first = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t second = PTHREAD_MUTEX_INITIALIZER;

void *thread_one(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&first);
    printf("Thread 1 locked first mutex\n");
    fflush(stdout);

    sleep(1);

    printf("Thread 1 waits for second mutex\n");
    fflush(stdout);
    pthread_mutex_lock(&second);

    pthread_mutex_unlock(&second);
    pthread_mutex_unlock(&first);

    return NULL;
}

void *thread_two(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&second);
    printf("Thread 2 locked second mutex\n");
    fflush(stdout);

    sleep(1);

    printf("Thread 2 waits for first mutex\n");
    fflush(stdout);
    pthread_mutex_lock(&first);

    pthread_mutex_unlock(&first);
    pthread_mutex_unlock(&second);

    return NULL;
}

int main(void)
{
    pthread_t t1;
    pthread_t t2;

    pthread_create(&t1, NULL, thread_one, NULL);
    pthread_create(&t2, NULL, thread_two, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
