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

void panic(const char *message) { perror(("%s\n", message)); exit(0); };
void error(const char *message) { printf("%s\n", message); exit(0); };
void warning(const char *message) { printf("%s\n", message); };

void buildRequest(char *buf, int port)
{ 
    //Gets host infomation
    char hostNameBuf[256];
    char *ipInfo;
    struct hostent *hostInfo;

    gethostname(hostNameBuf, sizeof(hostNameBuf));
    hostInfo = gethostbyname(hostNameBuf);                        //DEPRECATED

    //This converts the ip address in ASCII so this is only for debugging //TODO
    ipInfo = inet_ntoa(*((struct in_addr*)hostInfo->h_addr_list[0]));

    //Creates the HTTP request

    //Sets up the JSON
    char json[512];
    snprintf(json, sizeof(json), 
    "{"
        "\"PrivateIP\": \"%s\","
        "\"PrivatePort\": \"%i\""
    "}", ipInfo, port);

    //The Request itsself
    char message[1024];
    snprintf(message, sizeof(message),
    "POST / HTTP/1.1\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %li\r\n"
    "\r\n", strlen(json));

    strcat(message, json);
    strcpy(buf, message);
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

    int connectionState;
    do
    {
        connectionState = connect(clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
    } while (connectionState == -1);

    printf("Server found!\n");


    char buf[9999];
    buildRequest(buf, port);
    printf("%s\n------------------\n", buf);

    //Starts the conversation
    write(clientSocket, buf, strlen(buf));
    
    memset(buf, 0, sizeof(buf));
    read(clientSocket, buf, sizeof(buf));

    printf("%s\n", buf);

    bool running = true;
    while (running)
    {
        memset(buf, 0, sizeof(buf));
        read(clientSocket, buf, sizeof(buf));
    }

    close(clientSocket);
    shutdown(clientSocket, SHUT_RDWR);

    return 0;
}