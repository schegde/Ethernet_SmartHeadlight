#
# Makefile for Proxy Lab 
#
# You may modify is file any way you like (except for the handin
# rule). Autolab will execute the command "make" on your specific 
# Makefile to build your proxy from sources.
#
CC = gcc
CFLAGS = -Wall -O2 -g -DDRIVER -std=gnu99
LDFLAGS = -pthread

all: transfer

csapp.o: csapp.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c csapp.c

transfer.o: transfer.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c transfer.c

#cache.o: cache.c cache.h
#	$(CC) $(CFLAGS) -c cache.c

transfer: transfer.o csapp.o 

clean:
	rm *.o transfer


