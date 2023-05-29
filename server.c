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
#include <regex.h>

void panic(const char *message) { perror(message); exit(0); };
void error(const char *message) { printf("%s", message); exit(0); };
void warning(const char *message) { printf("%s", message); };

struct Client
{
    int fd;
    bool waiting;

    char *publicIp;
    int publicPort;

    char *privateIp;
    int privatePort;
};

struct Room
{
    int roomID;
    struct Client clientA;
    struct Client clientB;
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
    struct sockaddr_in clientInfo_global;
    socklen_t clientInfo_size = sizeof(clientInfo_global);

    int sd, sd_max;
    fd_set sdArray;

    int newClient;
    //int totalClients = 0;
    int maxClients = 1024;

    struct Client *activeClients = calloc(maxClients, sizeof(struct Client));

    //int activeClients[maxClients];
    //memset(activeClients, 0, sizeof(activeClients));

    //Some more client stuff
    int roomIndex = 0;
    int maxRoom = 100;
    struct Room *hotel = (struct Room *)calloc(maxRoom, sizeof(struct Room));

    char *buffer = (char*)calloc(9999, sizeof(char));

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
            sd = activeClients[i].fd;  

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
            if ((newClient = accept(serverSocket, (struct sockaddr*)&clientInfo_global, &clientInfo_size)) == -1)
            {
                printf("Error trying to accept\n");
                exit(-1);
            }

            //Struct for a client
            struct Client *clientInfo_local = malloc(sizeof(*clientInfo_local));
            clientInfo_local->fd = newClient;
            clientInfo_local->waiting = false;
            clientInfo_local->publicIp = inet_ntoa(clientInfo_global.sin_addr);
            clientInfo_local->publicPort = ntohs(clientInfo_global.sin_port);   

            //Announces the new connection
            printf("New connection at %s:%i\n", clientInfo_local->publicIp, clientInfo_local->publicPort);

            //Adds new client into to activeClients
            for (int i = 0; i < maxClients; i++)
            {
                if (activeClients[i].fd == 0)
                {
                    activeClients[i] = *clientInfo_local;
                    printf("%i\n", activeClients[i].fd);
                    free(clientInfo_local);
                    break;
                }
            }
        }

        //Old client request
        for (int i = 0; i < maxClients; i++)
        {
            //Disconnects client if needed  
            if (FD_ISSET(activeClients[i].fd, &sdArray))
            {
                //Announces disconnection
                printf("Client at %s:%i disconnected\n", activeClients[i].publicIp, activeClients[i].publicPort);
                
                close(activeClients[i].fd);  
                activeClients[i].fd = 0; 
            }
            //If there is still a client
            else if (activeClients[i].fd > 0 && !activeClients[i].waiting)
            {
                printf("%i\n", activeClients[i].fd);
                memset(buffer, 0, 9999);

                read(activeClients[i].fd, buffer, 9999);
                printf("%s\n", buffer);
                    
                //Parse the request
                char* token = strtok(buffer, "-");
                printf("%s\n", token);
                //Checks room
                switch (*token)
                {
                    case 'N':   //Create a room
                        struct Room *newHotelRoom = malloc(sizeof(*newHotelRoom));
                        activeClients[i].waiting = true;
                        newHotelRoom->clientA = activeClients[i];

                        //Ip
                        token = strtok(NULL, "-");
                        newHotelRoom->clientA.privateIp = token;
                        //Port
                        token = strtok(NULL, "-");
                        newHotelRoom->clientA.privatePort = atoi(token);
                        //Gives the room a id
                        token = strtok(NULL, "-");                       
                        newHotelRoom->roomID = atoi(token);

                        hotel[roomIndex] = *newHotelRoom;
                        printf("Created room!\n");
                        break;
                    case 'J':
                        printf("Attempting to join room\n");
                        //Ip
                        token = strtok(NULL, "-");
                        activeClients[i].privateIp = token;
                        //Port
                        token = strtok(NULL, "-");
                        activeClients[i].privatePort = atoi(token);

                        //Finds the room
                        struct Room activeRoom;
                        token = strtok(NULL, "-");
                        for (int j = 0; j < maxRoom; j++)
                        {
                            if (hotel[j].roomID == atoi(token))
                            {
                                printf("Room Found!\n");
                                hotel[j].clientB = activeClients[i];
                                activeRoom = hotel[j];
                                break;
                            }
                        }
                        //When found send a message to client A and B
                        printf("Tango Started\n");

                        //Client A
                        snprintf(buffer, 9999, "%s-%i-%s-%i", activeRoom.clientB.privateIp, activeRoom.clientB.privatePort, 
                            activeRoom.clientB.publicIp, activeRoom.clientB.publicPort);
                        //Sends message
                        write(activeRoom.clientB.fd, buffer, 9999);
                        memset(buffer, 0, 9999);
                        
                        printf("Sent message to client A\n");

                        //Client B
                        snprintf(buffer, 9999, "%s-%i-%s-%i", activeRoom.clientA.privateIp, activeRoom.clientA.privatePort, 
                            activeRoom.clientA.publicIp, activeRoom.clientA.publicPort);
                        //Sends message
                        write(activeRoom.clientB.fd, buffer, 9999);
                        printf("Sent message to client B\n");

                        printf("Tango Ended\n");
                        break;
                }
                //End communication
            }
        }     
    }

    close(serverSocket);
    //close(clientSocket);

    shutdown(serverSocket, SHUT_RDWR);
    //shutdown(clientSocket, SHUT_RDWR);

    return 1;
}