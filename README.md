# valueguesser
##### CPSC 360 Spring 2016 (Clemson University)

_A UDP client and server (implemented in C) that guesses a random value_

###Usage

The object of the valueGuesser is to guess the valueServer's
mystery value. After a value is correctly guessed the server
randomly generates another number between 0 and 1,000,000,000
for another client to connect and guess.

To build both binaries:

```make clean; make depend; make```

After the binaries have been built you can execute the following:

The valueGuesser uses a binary search algorithim to search
and determine the correct guess number.

To start the valueServer:

```
$ ./valueServer 
Usage: ./valueServer -p <serverPort> -v <initialValue> (optional)
```

You have to specify a port to start on. The -v parameter specifies
an initial value the server will use for the client to guess. After
the first round the value will be randomly generated again.

After the valueServer is started you can start the client: (valueGuesser)

```
$ ./valueGuesser
Usage: ./valueGuesser -s <server ip> -p1 <server port 1> -p2 <server port 2 (backup)
```

You have to specify a server to connect to along with a port. (-s and -p1) If the
first port specified is not available the guesser will try the second port. (-p2)
