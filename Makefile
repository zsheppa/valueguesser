CC=clang
CFLAGS=-Wall

DEPS=client.c server.c

all: valueGuesser valueServer

clean:
	$(RM) -v valueGuesser valueServer *.o

depend: client.o server.o

valueGuesser: client.o
	$(CC) $(CFLAGS) -o valueGuesser client.c

valueServer: server.o
	$(CC) $(CFLAGS) -o valueServer server.c
    
client.o:
	$(CC) $(CFLAGS) -c client.c
    
server.o:
	$(CC) $(CFLAGS) -c server.c

