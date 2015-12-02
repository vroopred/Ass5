CC = gcc
CFLAGS = -Wall -g

all: clean minls minget

minls: minls.o 
	$(CC) $(CFLAGS) -o minls minls.o 

minget: minget.o
	$(CC) $(CFLAGS) -o minget minget.o

minls.o: 
	$(CC) $(CFLAGS) -c -o minls.o minls.c

minget.o:
	$(CC) $(CFLAGS) -c -o minget.o minget.c

clean:
	rm -rf *.o minls minget 