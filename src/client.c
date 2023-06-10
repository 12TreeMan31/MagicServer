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

//Gets the ip that the client is using to connect to server
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

void initP2Pconnection(int *clientSocket, struct sockaddr_in serverInfo, char roomType, int port)
{
    char messageBuffer[2048];
    char *clientInfo, *token;
    int roomID;

    //Asks to room id
    printf("Room Name: ");
    scanf("%i", &roomID);
    //Creates the response
    clientInfo = getHostInfo();
    snprintf(messageBuffer, sizeof(messageBuffer),
        "%c-%s-%i-%i", roomType, clientInfo, port, roomID);

    //Starts talking to the server to make the connection happen
    write(*clientSocket, messageBuffer, strlen(messageBuffer));
    printf("Sent connection request %s\n", messageBuffer);
    memset(messageBuffer, 0, sizeof(messageBuffer));

    //Gets respanse back with another clients ip and port
    read(*clientSocket, messageBuffer, sizeof(messageBuffer));
    printf("Starting connection to client\n");

    //Parses the message and modifys the server infomation to point to other client 
    
    //Private
    //IP
    token = strtok(messageBuffer, "-");
    inet_aton(token, &serverInfo.sin_addr);
    printf("Private IP: %s\n", token);
    //Port
    token = strtok(NULL, "-");
    serverInfo.sin_port = htons(atoi(token));
    printf("Private Port: %s\n", token);

    //Public
    //IP
    token = strtok(NULL, "-");
    inet_aton(token, &serverInfo.sin_addr);
    printf("Public IP: %s\n", token);

    //Port
    token = strtok(NULL, "-");
    serverInfo.sin_port = htons(atoi(token));
    printf("Public Port: %s\n", token);

    //Trys to connect to the other client
    printf("Waiting on client...\n");
    attemptConnection(clientSocket, serverInfo);
    printf("Connected to client!\n");
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
    inet_aton("98.226.56.248", &serverInfo.sin_addr);

    //Configures socket options 
    int opt = true;  
    if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    { 
        panic("Could not configure socket"); 
    }

    //Connect to server
    printf("Looking for server...\n");
    attemptConnection(&clientSocket, serverInfo);
    printf("Server found!\n");

    char messageBuffer[2048];
    char userInput;

    bool running = true;
    while (running) 
    {
        //Clears buffers
        memset(messageBuffer, 0, sizeof(messageBuffer));

        //Input
        printf("> ");
        scanf("%c", &userInput);
        fflush(stdin);
        
        switch (userInput)
        {
            case 'j':   //Join a room
                initP2Pconnection(&clientSocket, serverInfo, 'J', port);
                break;
            case 'n':   //Create a new room
                initP2Pconnection(&clientSocket, serverInfo, 'N', port);
                break;
            case 'q':   //Quits
                close(clientSocket);
                shutdown(clientSocket, SHUT_RDWR);
                break;
            default:
                printf("Not a command\n");
                break;
        }
    }
    return 0;
}