#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <regex.h>

void panic(const char *message) { perror((message)); exit(0); };
void error(const char *message) { printf("%s\n", message); exit(0); };
void warning(const char *message) { printf("%s\n", message); };


//Gets the ip that the client is using th connect to server
char* getHostInfo()
{
    //Gets host infomation
    char hostNameBuf[256];
    struct hostent *hostInfo;

    gethostname(hostNameBuf, sizeof(hostNameBuf));
    hostInfo = gethostbyname(hostNameBuf);                        //DEPRECATED

    //This converts the ip address to ASCII
    return (inet_ntoa(*((struct in_addr*)hostInfo->h_addr_list[0])));
    //printf("%s", ipInfo);
}

void attemptConnection(int *clientSocket, struct sockaddr_in serverInfo)
{
    int connectionState;
    do
    {
        connectionState = connect(*clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
    } while (connectionState == -1);
}

void initP2Pconnection()
{
    //Creates the response
    scanf("Room ID: %i", &roomID);
    ipInfo = getHostInfo();
    snprintf(messageBuffer, sizeof(messageBuffer),
        "J-%s-%i-%i",ipInfo, port, 123);

    //Starts talking to the server to make the connection happen
    write(clientSocket, messageBuffer, strlen(messageBuffer));
    printf("Sent connection request %s\n", messageBuffer);
    memset(messageBuffer, 0, sizeof(messageBuffer));

    //Gets respanse back with another clients ip's and port's
    read(clientSocket, messageBuffer, sizeof(messageBuffer));
    printf("Starting connection to client\n");
    //Creates more buffers so message can be parsed with regex
    strcpy(tempBuffer, messageBuffer);
    regexec(&readMessageIP, messageBuffer, 0, NULL, 0);
    regexec(&readMessagePort, tempBuffer, 0, NULL, 0);
    printf("Port: %s\n", tempBuffer);
    printf("IP: %s\n", messageBuffer);
    //Rebinds the active socket other client
    serverInfo.sin_port = htons(atoi(tempBuffer));
    inet_aton(messageBuffer, &serverInfo.sin_addr);
    //Trys to connect to the other client
    connect(clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
}


int main(int argc, char *argv[])
{
    int port = -1;
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port < 0) error("Please enter a valid port");
    }
    else
    {
        error("Did not specify a port");
    }

    // Make socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Sets up info
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);
    serverInfo.sin_addr.s_addr = htonl(0x7F000001);

    //Configures socket options 
    int opt = true;  
    if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    { 
        panic("Could not configure socket"); 
    }

    printf("Looking for server...\n");

    attemptConnection(&clientSocket, serverInfo);

    printf("Server found!\n");

    char messageBuffer[2048];
    char userInput, *ipInfo;
    int roomID;

    regex_t readMessageIP, readMessagePort;
    regcomp(&readMessageIP, ":.*:", 0);
    regcomp(&readMessagePort, "|.*|", 0);

    bool running = true;
    while (running) 
    {
        //Clears buffers
        memset(messageBuffer, 0, sizeof(messageBuffer));
        //
        char tempBuffer[1024];
        printf("> ");
        scanf("%c", &userInput);
        switch (userInput)
        {
            case 'j':   //Join a room
                //Creates the response
                scanf("Room ID: %i", &roomID);
                ipInfo = getHostInfo();
                snprintf(messageBuffer, sizeof(messageBuffer),
                    "J-%s-%i-%i",ipInfo, port, 123);

                //Starts talking to the server to make the connection happen
                write(clientSocket, messageBuffer, strlen(messageBuffer));
                printf("Sent connection request %s\n", messageBuffer);
                memset(messageBuffer, 0, sizeof(messageBuffer));

                //Gets respanse back with another clients ip's and port's
                read(clientSocket, messageBuffer, sizeof(messageBuffer));
                printf("Starting connection to client\n");
                //Creates more buffers so message can be parsed with regex
                strcpy(tempBuffer, messageBuffer);
                regexec(&readMessageIP, messageBuffer, 0, NULL, 0);
                regexec(&readMessagePort, tempBuffer, 0, NULL, 0);
                printf("Port: %s\n", tempBuffer);
                printf("IP: %s\n", messageBuffer);
                //Rebinds the active socket other client
                serverInfo.sin_port = htons(atoi(tempBuffer));
                inet_aton(messageBuffer, &serverInfo.sin_addr);
                //Trys to connect to the other client
                connect(clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
                break;
            case 'n':   //Create a new room
                //Creates the response
                scanf("Room ID: %i", &roomID);
                ipInfo = getHostInfo();
                snprintf(messageBuffer, sizeof(messageBuffer),
                    "N-%s-%i-%i",ipInfo, port, 123);

                //Starts talking to the server to make the connection happen
                write(clientSocket, messageBuffer, strlen(messageBuffer));
                printf("Sent connection request %s\n", messageBuffer);
                memset(messageBuffer, 0, sizeof(messageBuffer));

                //Gets respanse back with another clients ip's and port's
                read(clientSocket, messageBuffer, sizeof(messageBuffer));
                printf("Starting connection to client\n");
                //Creates more buffers so message can be parsed with regex
                strcpy(tempBuffer, messageBuffer);
                regexec(&readMessageIP, messageBuffer, 0, NULL, 0);
                regexec(&readMessagePort, tempBuffer, 0, NULL, 0);
                printf("Port: %s\n", tempBuffer);
                printf("IP: %s\n", messageBuffer);
                //Rebinds the active socket other client
                serverInfo.sin_port = htons(atoi(tempBuffer));
                inet_aton(messageBuffer, &serverInfo.sin_addr);
                //Trys to connect to the other client
                connect(clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
                break;
            case 'q':   //Quits
                close(clientSocket);
                shutdown(clientSocket, SHUT_RDWR);
                break;
            default:
                printf("%c", userInput);
                break;
        }
    }
    return 0;
}