#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>

#define MAX_PEER_CONNECTIONS 1024
#define BUFFER_SIZE 2048

void PANIC(const char *message) { perror(message); exit(0); };
void ERROR(const char *message) { printf("%s\n", message); exit(0); };
void WARNING(const char *message) { printf("%s\n", message); };

//Your infomation
struct thisClient
{
    char *username;
    int publicKey, privateKey;
};

//Info about the other users
struct Peer
{
    int fd;
    char *username;
    int publicKey;
    bool waiting;

    struct sockaddr_in private;
    struct sockaddr_in public;
};

//This is for server operation and holds connection infomation
struct Server
{
    int socket;
    struct sockaddr_in serverLocation;
    //0 - Client, 1 - Server, 2 - MixedMode
    int serverMode;

    struct Peer *activePeers;
};

struct Party
{
    char* partyName;
    char* password;

    struct Peer *partyMembers;
};

//static struct Server *serverMain;


void argument_check(int argc, char *argv[], struct Server *connectionInfo)
{
    for (int i = 1; i < argc; i++)                      //Loops though all arguments
    {
        switch((char)argv[i][0])
        {
        case '-':
            switch ((char)argv[i][1])
            {
            case 'p':       //Ports
                i++;
                unsigned short port = atoi(argv[i]);
                if (port < 1023 || argv[i][0] == '-') ERROR("Please enter a valid port");
                connectionInfo->serverLocation.sin_port = htons(port);
                break;
            case 'a':       //IP address
                i++;
                inet_aton(argv[i], &connectionInfo->serverLocation.sin_addr);
                    break;       
            case 's':       //Server mode
                connectionInfo->serverMode = 1;
                break;
            default:        //Error
                ERROR("Invalid switch");
                break;
            }
            break;
        default:    //Error
            ERROR("Invalid switch");
            break;
        }
    }
}

//Parses a request into an array - TODO: Make them HTTP request
void parse_request(char *message, char *destination[])
{
    destination[0] = strtok(message, "-");
    for (int i = 1; i < 4; i++) 
    {
        destination[i] = strtok(NULL, "-");
    }
}

void build_request(char *buffer, char *data[])
{
    snprintf(buffer, strlen(buffer),
        "%s-%s-%s-%s", data[0], data[1], data[2], data[3]);
}

void create_party()
{

}


//Client
void attempt_connection(int *socket, struct sockaddr_in serverLocation)
{
    int connectionState;
    do 
    {
        connectionState = connect(*socket, (struct sockaddr*)&serverLocation, sizeof(serverLocation));
    } while (connectionState == -1);
}


//Gets the ip that the client is using to connect to server - Fix: gethostbyname() is deprecated
char* get_private_ip()
{
    //Gets host infomation
    char hostNameBuf[256];
    struct hostent *hostInfo;

    gethostname(hostNameBuf, sizeof(hostNameBuf));
    hostInfo = gethostbyname(hostNameBuf);                        //DEPRECATED

    //This converts the ip address to ASCII
    return (inet_ntoa(*((struct in_addr*)hostInfo->h_addr_list[0])));
    //return (struct in_addr*)hostInfo->h_addr_list[0];
}

//Creates or joins a room with the help of a homeserver - TODO: Add a room password and encrytion 
void start_p2p_connection(char *buffer, char *partyType, struct Server *connectionInfo)
{
    char partyName[10], *parsedMessage[4];

    printf("Party Name: ");
    scanf("%s", partyName);

    //partyType - privateIP - privatePort - partyName
    snprintf(buffer, BUFFER_SIZE,
        "%s-%s-%i-%s", partyType, get_private_ip(), ntohs(connectionInfo->serverLocation.sin_port), partyName);

    //Post
    write(connectionInfo->socket, buffer, BUFFER_SIZE);
    printf("Sent connection request! - %s\n", buffer);
    memset(buffer, 0, BUFFER_SIZE);

    //Get
    printf("Waiting for server response...\n");
    read(connectionInfo->socket, buffer, BUFFER_SIZE);
    printf("Recived - %s\n", buffer);

    //privateIP - privatePort - publicIP - publicPort
    parse_request(buffer, parsedMessage);

    //ADD THIS


    //For right now look at local ip
    printf("Attempting connection to peer...\n");
    attempt_connection(&connectionInfo->socket, connectionInfo->serverLocation);
    printf("Success!\n");
}


int main(int argc, char *argv[])
{
    static struct Server *connectionInfo;

    //Check for arguments
    if (argc > 1)
    {
        connectionInfo = malloc(sizeof(*connectionInfo));
        connectionInfo->serverLocation.sin_family = AF_INET;
        argument_check(argc, argv, connectionInfo);
    }
    else
    {
        ERROR("Insufficient arguments");
    }

    //Creates socket
    connectionInfo->socket = socket(AF_INET, SOCK_STREAM, 0);

    //Configures socket options 
    int opt = true;  
    if (setsockopt(connectionInfo->socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    { 
        PANIC("Could not configure socket"); 
    }

    printf("Connecting to server at %s:%i\n", inet_ntoa(connectionInfo->serverLocation.sin_addr), ntohs(connectionInfo->serverLocation.sin_port));

    //Important
    struct thisClient *You = malloc(sizeof(*You));
    char *buffer = (char*)calloc(9999, sizeof(char));
    char userInput;

    bool running = true;
    while (running)
    {
        switch (connectionInfo->serverMode)
        {
        case 0: //Client
            //Connect to server
            printf("Looking for server...\n");
            attempt_connection(&connectionInfo->socket, connectionInfo->serverLocation);
            printf("Server Found\n");

            //User Input
            printf("> ");
            scanf("%c", &userInput);
            fflush(stdin);
            
            //Party stuff
            switch (userInput)
            {
            case 'j':   //Join a party
                start_p2p_connection(buffer, "J", connectionInfo);
                break;
            case 'n':   //Create a new party
                start_p2p_connection(buffer, "N", connectionInfo);
                break;
            case 'q':   //Quits
                close(connectionInfo->socket);
                shutdown(connectionInfo->socket, SHUT_RDWR);
                break;
            default:
                printf("Not a command\n");
                break;
            }
            break;
        case 1: //Server

            break;
        case 2: //Mixed

            break;

        default:
            ERROR("ServerMode was not valid");
            break;
        }
    }

    //Code for creating room



    //Normal communication 

}