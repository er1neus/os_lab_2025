CC = gcc
CFLAGS = -Wall -Wextra -O2
PTHREAD = -pthread

all: client server

netlib.o: netlib.c netlib.h
	$(CC) $(CFLAGS) -c netlib.c -o netlib.o

libnet.a: netlib.o
	ar rcs libnet.a netlib.o

client: client.c netlib.h libnet.a
	$(CC) $(CFLAGS) client.c -L. -lnet $(PTHREAD) -o client

server: server.c netlib.h libnet.a
	$(CC) $(CFLAGS) server.c -L. -lnet -o server

clean:
	rm -f client server netlib.o libnet.a
