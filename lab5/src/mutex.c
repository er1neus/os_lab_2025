#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 4
#define ITERATIONS 5000000

static long long counter = 0;

#ifdef USE_MUTEX
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void *worker(void *arg)
{
    (void)arg;

    for (long long i = 0; i < ITERATIONS; i++) {
#ifdef USE_MUTEX
        pthread_mutex_lock(&mutex);
#endif

        counter++;

#ifdef USE_MUTEX
        pthread_mutex_unlock(&mutex);
#endif
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Expected: %lld\n", (long long)THREADS * ITERATIONS);
    printf("Actual:   %lld\n", counter);

#ifdef USE_MUTEX
    pthread_mutex_destroy(&mutex);
#endif

    return 0;
}
