#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

int main(int argc, char * argv[]){
    //create TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
    {
        cerr << "Error with Socket" << endl;
        exit(-1);
    }

    //Establish connection
    char *IPAddr = argv[1]; //passed IP address
    unsigned short servPort = atoi(argv[2]); //convert string to int

    //Convert dotted decimal address to int
    unsigned long servIP;
    int status = inet_pton(AF_INET, IPAddr, (void*)&servIP);
    if(status <= 0) exit(-1);

    //Set the fields
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = servIP;
    servAddr.sin_port = htons(servPort);

    //Connect
    status = connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if(status < 0)
    {
        cerr << "Error with connect" << endl;
        exit(-1);
    }

    //Send data
    int turn = 0;
    bool win = false;
    if(turn == 0)
    {
        cout << "Welcome to the Number Guessing Game!" << endl;
        cout << "Enter your name: ";
        string msgStr;
        getline (cin,msgStr);

        // send name to server
        char msg[9999];
        if(msgStr.length() >= 9999) exit(-1);
        strcpy(msg,msgStr.c_str());
        int bytesSent = send(sock,(void*) msg, 9999, 0);
        if(bytesSent != 9999)
        {
            cerr << "ByteSent not equal to  9999" << endl;
            exit(-1);
        }
        //update turn
        turn ++;
    }

    while(!win)
    {
        //Validate guess
        cout << "\n";
        cout << "Turn: " << turn << "\n";
        cout << "Enter a guess: ";
        long guess;
        while(!(cin >> guess) || guess < 0 || guess > 9999)
        {
            cout << "\nGuess must be a positve number between  0 and 9999." << endl;
            cout<< "Enter a guess: ";
            cin.clear();
            cin.ignore(100, '\n');
        }
        turn++;

        //Send guess
        long netwokrInt = htonl(guess);
        int intBytesSent = send(sock,(void *) &netwokrInt,sizeof(long), 0);
        if(intBytesSent != sizeof(long)) exit(-1);

        //Recive difference
        int bytesLeft = sizeof(long);
        long networkInt;
        char *bp = (char *) &networkInt;
        while(bytesLeft)
        {
            int bytesRecv = recv(sock, (void*)bp, bytesLeft,0);
            if(bytesRecv <= 0) exit(-1);
            bytesLeft = bytesLeft - bytesRecv;
            bp = bp + bytesRecv;
        }
        long hostInt = ntohl(networkInt);
        cout << "Result of guess: " << hostInt << "\n";

        //Exit loop if guess is correct
        if(hostInt == 0)
        {
            win = true;
            //Recieve congratulations message and leaderboard
            int bytesLeft = 9999;
            char buffer[9999];
            char *bp = buffer;
            while(bytesLeft > 0)
            {
                int bytesRecv = recv(sock, (void *)bp, bytesLeft, 0);
                if(bytesRecv <= 0)
                {
                    perror("The following error occured: ");
                    exit(-1);
                }
                bytesLeft = bytesLeft - bytesRecv;
                bp = bp +  bytesRecv;
                }
                cout << string(buffer) << endl;
            }
        }
    //Close socket
    close(sock);
    return 0;
}
