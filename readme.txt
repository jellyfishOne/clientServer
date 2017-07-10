This client/server number guessing game runs on Mac OS X terminal.

server.cpp takes a port number as command line argument.

client.cpp takes an IP address and a port number as command line argument. 

The program lets users play a simple number guessing game. The client connects
to the server and creates a username. The server will save the username and
generates a random number for the client to guess. The client sends a guess
to the server and the server responds with how close the client was to guessing
the secret number. After the client guesses the correct number, the server
updates a leader board and sends the results to the client.
