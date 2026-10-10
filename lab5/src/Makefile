CC = gcc
CFLAGS = -Wall -Wextra -O0 -pthread

all: mutex_no_mutex mutex_with_mutex factorial deadlock

mutex_no_mutex: mutex.c
	$(CC) $(CFLAGS) mutex.c -o mutex_no_mutex

mutex_with_mutex: mutex.c
	$(CC) $(CFLAGS) -DUSE_MUTEX mutex.c -o mutex_with_mutex

factorial: factorial.c
	$(CC) $(CFLAGS) factorial.c -o factorial

deadlock: deadlock.c
	$(CC) $(CFLAGS) deadlock.c -o deadlock

clean:
	rm -f mutex_no_mutex mutex_with_mutex factorial deadlock
