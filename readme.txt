This client/server number guessing game runs on Mac OS X terminal.

server.cpp takes a port number as command line arguments.

client.cpp takes an IP address and a port number as command line argument.

The program let users play a simple number guessing game. The client connects
to the server, the server generates a random number for the client to guess, the
client then sends a guess and the server responds with how close the client was to
guessing the number. After the client guesses the correct number, the server
updates a leader board and sends the results to the client.
