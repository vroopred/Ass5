CC = gcc
CFLAGS = -Wall -g

all: clean minls 

minls: minls.o 
	$(CC) $(CFLAGS) -o minls minls.o 


minls.o: 
	$(CC) $(CFLAGS) -c -o minls.o minls.c


clean:
	rm -rf *.o minls minget 