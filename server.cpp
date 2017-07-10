#include <pthread.h>
#include <iostream>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <ctime>
#include <cstdlib>
using namespace std;

//Sets the maximum number of pending incoming requests
const int MAXPENDING = 10;

//Struct used to pass into thread function
struct ThreadArgs{
    int clientSock;
};

//Struct used to track game
struct game{
    string name;
    int turn;
    int answer;
};

//Array for leaderbord
game leaderbord[3];

//Declare semaphore
sem_t *semaphore;

//update leaderboard
void *updateLB(void *args)
{
    struct game *place = (struct game *)args;
    bool control = false;
    for(int i = 0; i < 3; i++)
    {
        //Check for empty spot
        if(leaderbord[i].name.empty() && !control){
            leaderbord[i].turn = place->turn;
            leaderbord[i].name = place->name;
            control = true;

        } else if(place->turn < leaderbord[i].turn)
        {
            //swap
            game *temp = new game;
            temp -> turn = leaderbord[i].turn;
            temp -> name = leaderbord[i].name;

            leaderbord[i].turn = place->turn;
            leaderbord[i].name = place->name;

            place->turn = temp ->turn;
            place->name = temp ->name;
        }
    }

    delete place;
    return NULL;
}

//Communicate with client
void *communicate(void  *args)
{
    //Extract socket file descriptor from argument
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    int clientSock = ((ThreadArgs*)threadArgs)->clientSock;
    delete threadArgs;

    //Create new "game" struct
    game *gameStart = new game;
    gameStart -> turn = -1;

    //communicate with client while they are playing the game
    bool win = false;
    while(!win)
    {
        //Recive name and save name to structure and
        //generate random number
        if(gameStart ->turn < 0)
        {
            //Recieve name
            int bytesLeft = 9999;
            char buffer[9999];
            char *bp = buffer;
            while(bytesLeft > 0)
            {
                int bytesRecv = recv(clientSock, (void *)bp, bytesLeft, 0);
                if(bytesRecv <= 0)
                {
                    perror("The following error occured: ");
                    pthread_exit(NULL);
                }
                bytesLeft = bytesLeft - bytesRecv;
                bp = bp +  bytesRecv;
            }
            //save name to struct
            gameStart -> name = string(buffer);

            //generate random number 0000 to 9999
            srand(time(NULL));
            gameStart -> answer = rand() % 10000;
            cout << "Random number generated for " << gameStart->name;
            cout<< ": "<<gameStart -> answer << endl;

            //update turn
            gameStart -> turn += 1;
        }
        else
        {
            //Recive integer guess
            int bytesLeft = sizeof(long);
            long clientInt;
            char *bp = (char *) &clientInt;
            while(bytesLeft)
            {
                int bytesRecv = recv(clientSock, (void*)bp, bytesLeft,0);
                if(bytesRecv <= 0) pthread_exit(NULL);;
                bytesLeft = bytesLeft - bytesRecv;
                bp = bp + bytesRecv;
            }
            long clientGuess = ntohl(clientInt);

            //Calculate how close guess was
            int diff = 0;
            int answer = gameStart -> answer;
            while(clientGuess > 0)
            {
                int a = clientGuess % 10;
                int b = answer % 10;
                diff += abs(a - b);
                clientGuess /=10;
                answer /= 10;
            }

            //Send how close guess was
            long hostInt;
            hostInt = diff;
            long netwokrInt = htonl(hostInt);
            int bytesSent = send(clientSock,(void *) &netwokrInt,sizeof(long), 0);
            if(bytesSent != sizeof(long)) pthread_exit(NULL);

            //update turn
            gameStart -> turn += 1;

            //Exit loop if guess is correct
            if(diff == 0)
                {
                    win = true;

                    //save number of turns
                    int result = gameStart-> turn;

                    //update leaderboard
                    sem_wait(semaphore);
                    updateLB(gameStart);
                    sem_post(semaphore);

                    //send congratulations message and leaderboard
                    ostringstream ss;
                    string ssString;
                    ss << "Congratulations! ";
                    ss << "It took " << result << " turns to guess the number!\n";
                    ss <<"Leaderboard\n";
                    for(int i = 0; i < 3; i++)
                    {
                        if(!leaderbord[i].name.empty()){
                            ss << i+1 <<"."<< leaderbord[i].name <<" " <<leaderbord[i].turn << "\n";
                        }
                    }
                    ssString = ss.str();
                    char congrats[9999];
                    if(ssString.length() >= 9999) pthread_exit(NULL);;
                    strcpy(congrats,ssString.c_str());
                    bytesSent = send(clientSock,(void*) congrats, 9999, 0);
                    if(bytesSent != 9999)
                    {
                        cerr << "ByteSent not equal to  50" << endl;
                        pthread_exit(NULL);;
                    }
                }
        }
    }
    return NULL;
}


int main(int argc, char * argv[]){

    //Create TCP socket
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		cerr << "Error with socket" << endl;
		exit(-1);
    }

    //setting the port
    int portNum = atoi(argv[1]);
    unsigned short servPort = portNum;
    struct sockaddr_in servAddr;

    //Set the fields
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    //bind
    int status = bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if(status < 0)
    {
        cerr << "Error with bind" <<endl;
        exit(-1);
    }

    //Set socket to listen
    status = listen(sock, MAXPENDING);
    if(status < 0)
    {
        cerr << "Error with listen" << endl;
        exit(-1);
    }
    cout << "Server is listening"<< endl;

    //Initiate Semephores
    //sem_init(&wait,0, 1);
    semaphore = sem_open("/semaphore", O_CREAT, 0, 1);
    //Connect
    while(true) //can only handle 10 request at a time
    {
        //Accept connection
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSock = accept(sock,(struct sockaddr*)&clientAddr, &addrLen);
        if(clientSock < 0)
        {
            perror("The following error occured: ");
            exit(-1);
        }
        //Create and initialize argument struct
        ThreadArgs *threadArgs= new ThreadArgs;
        threadArgs -> clientSock = clientSock;

        //Create client thread
        pthread_t threadID;
        int state = pthread_create(&threadID, NULL, &communicate, (void*)threadArgs);
        if(state != 0)
        {
            perror("The Following error occured: ");
            exit(-1);
        }
   }
    return 0;
}
