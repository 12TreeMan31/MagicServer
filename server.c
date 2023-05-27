#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>       

#include <cjson/cJSON.h>

void panic(const char *message) { perror(message); exit(0); };
void error(const char *message) { printf("%s", message); exit(0); };
void warning(const char *message) { printf("%s", *message); };

struct Room
{
    int roomID;
    struct in_addr clientA_Public;
    struct in_addr clientA_Private;

    struct in_addr clientB_Public;
    struct in_addr clientB_Private;
};

int main(int argc, char *argv[])
{
    //Args
    int port = -1;
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port < 0)
        {
            panic("Please enter a valid port");
        }
    }
    else
    {
        panic("Did not specify a port");
    }

    //Creates the socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Server info
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);

    //Configures socket options 
    int opt = true;  
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    { 
        panic("Could not configure socket"); 
    }

    // Binds serverInfo sin_addr
    if (bind(serverSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1)
    {
        panic("Failed to bind socket");
    }

    //Listen
    listen(serverSocket, 1024);
    
    // Client info
    struct sockaddr_in clientInfo;
    socklen_t clientInfo_size = sizeof(clientInfo);

    int sd, sd_max;
    fd_set sdArray;

    int newClient;
    int totalClients = 0;
    int maxClients = 1024;
    int activeClients[maxClients];
    memset(&activeClients, 0, sizeof(activeClients));


    bool running = true;
    while (running)
    {
        // Clears sockets
        FD_ZERO(&sdArray);

        // Adds server
        FD_SET(serverSocket, &sdArray);
        sd_max = serverSocket;

        // Re-adds all the clients
        for (int i = 0; i < maxClients; i++)
        {
            // Takes a socket from active clients 
            sd = activeClients[i];  
 
            // If socket exsists add it to sdArray
            if(sd > 0)
            {
                FD_SET(sd , &sdArray); 
            }

            // Highest file descriptor number, need it for the select function 
            if(sd > sd_max)  
            {
                sd_max = sd;  
            }
        }

        //Waits for something to happen
        if (select(sd_max + 1, &sdArray, NULL, NULL, NULL) == -1)
        {
            printf("Failed to select\n");
            //exit(-1);
        }

        //It something happens to serverSocket
        if (FD_ISSET(serverSocket, &sdArray))
        {
            //Adds a new client
            if ((newClient = accept(serverSocket, (struct sockaddr*)&clientInfo, &clientInfo_size)) == -1)
            {
                printf("Error trying to accept\n");
                exit(-1);
            }
 
            //Announces the new connection
            printf("New connection at %s:%i\n", inet_ntoa(clientInfo.sin_addr) , ntohs(clientInfo.sin_port));

            //Adds new connection to sdArray
            for (int i = 0; i < maxClients; i++)
            {
                if (activeClients[i] == 0)
                {
                    activeClients[i] = newClient;
                    break;
                }
            }
        }

        //Old client request
        for (int i = 0; i < maxClients; i++)
        {
            sd = activeClients[i];

            //Checks to see if client is still connected 
            if (FD_ISSET(sd, &sdArray))
            {
                //Disconnects client
                getpeername(sd , (struct sockaddr*)&clientInfo , &clientInfo_size);
                printf("Client at %s:%i disconnected\n", inet_ntoa(clientInfo.sin_addr) , ntohs(clientInfo.sin_port));
                
                close(sd);  
                activeClients[i] = 0; 
            }
            else
            {
                //Manage it
                if ((sd = activeClients[i]) > 0)
                {
                    char buffer[1024];
                    read(sd, buffer, sizeof(buffer));
                    printf("%s\n", buffer);

                    char *message = "HTTP/1.1 200 OK\r\n";
                    write(sd, message, strlen(message));
                }
            }
        }     
    }

    close(serverSocket);
    //close(clientSocket);

    shutdown(serverSocket, SHUT_RDWR);
    //shutdown(clientSocket, SHUT_RDWR);

    return 1;
}
