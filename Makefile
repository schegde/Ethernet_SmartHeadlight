#Makefile


CC = gcc
CFLAGS = -Wall -O2 -g -DDRIVER -std=gnu99
LDFLAGS = -pthread

all: transfer_app

csapp.o: csapp.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c csapp.c

transfer.o: transfer.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c transfer.c

transfer_app.o: transfer_app.c
	$(CC) $(LDFLAGS) $(CFLAGS) -c transfer_app.c


transfer_app: transfer_app.o transfer.o csapp.o 

clean:
	rm *.o transfer_app


#***************************************************

# CC = gcc
# CFLAGS = -Wall -O2 -g -DDRIVER -std=gnu99
# LDFLAGS = -pthread

# all: transfer

# csapp.o: csapp.c
# 	$(CC) $(LDFLAGS) $(CFLAGS) -c csapp.c

# transfer.o: transfer.c
# 	$(CC) $(LDFLAGS) $(CFLAGS) -c transfer.c

# transfer: transfer.o csapp.o 

# clean:
# 	rm *.o transfer;
