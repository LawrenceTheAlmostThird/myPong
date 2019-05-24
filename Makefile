# makefile for Linked List program

CC = gcc
CFLAGS = -O -Wall 
LFLAGS = -lncurses -lpthread -lm

all: pong_client pong_server

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

pong_client: client.c csapp.o
	$(CC) $(CFLAGS) -c -o client.o client.c
	$(CC) -o pong_client client.o csapp.o $(LFLAGS)

pong_server: server.c csapp.o
	$(CC) $(CFLAGS) -c -o server.o server.c
	$(CC) -o pong_server server.o csapp.o $(LFLAGS)

clean:
	rm -f pong_client *.o
