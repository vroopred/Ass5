CC = gcc
CFLAGS = -Wall -g

all: clean minls minget

minls: minls.o lib.o
	$(CC) $(CFLAGS) -o minls minls.o lib.o 

minget: minget.o lib.o
	$(CC) $(CFLAGS) -o minget minget.o lib.o

lib.o: 
	$(CC) $(CFLAGS) -c -o lib.o lib.c	

minls.o: 
	$(CC) $(CFLAGS) -c -o minls.o minls.c

minget.o:
	$(CC) $(CFLAGS) -c -o minget.o minget.c

clean:
	rm -rf *.o minls minget 
