#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//Network
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <sys/types.h> //getaddrinfo
#include <netdb.h>     //getaddrinfo
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <poll.h>
//Threading
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 9999
#define MAX_CONNECTIONS 1024

#define DEFAULT_USERNAME "Anonymous\0"
#define DEFAULT_SERVER_PORT 7168
#define CLIENT_PORT 7169

static bool debugMode = false;

// Everything is a peer and it hold important infomation about the session
struct peer
{
    int *fd;
    //Max length of username is 20 + \0
    char username[21];
    int publicKey;

    struct sockaddr_in private;
    struct sockaddr_in public;
};

//This is shared between all threads
struct server
{
    int servSock;
    int clintSock;
    int totalActiveConnections;
    struct peer atvPeers[MAX_CONNECTIONS];

    struct sockaddr_in info;
};


int compress_array(struct peer *fds[], int length)
{
    return 0;

}

//

int connection_close(int *fd)
{
    struct sockaddr_in clientInfo;
    socklen_t clientInfo_size = sizeof(clientInfo);

    getpeername(*fd, (struct sockaddr *)&clientInfo , &clientInfo_size);
    printf("Connection lost at %s:%i\n", inet_ntoa(clientInfo.sin_addr) , ntohs(clientInfo.sin_port));
    close(*fd);
    *fd = -1;

    return 0;  
}
 
//Accepts ALL new connections at fd
/*int connection_receive(int fd, struct sockaddr_in *clientInfo)
{
    socklen_t clientInfo_size = sizeof(*clientInfo);
    int newClient;
    do
    {
        newClient = accept(fd, (struct sockaddr *)clientInfo, &clientInfo_size);
        if (newClient == -1) break;

        //Announces the new connection
        printf("New connection at %s:%i\n", inet_ntoa(clientInfo->sin_addr) , ntohs(clientInfo->sin_port));

    } while (newClient != -1);
    
    return 0;
}*/

//

int data_parse(char *message, char *destination[], short dataBits, char *delimiter)
{
    destination[0] = strtok(message, delimiter);
    for (short i = 1; i < dataBits; i++) 
    {
        destination[i] = strtok(NULL, delimiter);
    }
    return 0;
}

int str_format(char *buffer, size_t buffLength, char *data[], int dataCount)
{
    char *tBuffer = (char *)calloc(BUFFER_SIZE, sizeof(char));

    for (int i = 0; i < dataCount - 1; i++)
    {
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, buffLength, "%s-%s", tBuffer, data[i]);
        strcpy(tBuffer, buffer);
    }
    free(tBuffer);
    return 0;
}

int data_read(int *fd, char *buffer)
{
    switch (recv(*fd, buffer, BUFFER_SIZE, 0))
    {
    case 0:
        // Left Empty on purpose - Client Disconnects 
        break;
    case -1:
        perror("Failed to get message: ");
        break;

    default:
        // Left Empty on purpose - Got message - TODO Send signal that everything was good
        return 0;
    }

    //If message failed to read
    //connection_close(fd);
    return -1;
}

int data_write()
{
    return 0;
}

//

//Polls for events
void *inbound_io(void *master_origin)
{
    struct server *master = (struct server *)master_origin;

    char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
    char **pBuffer = (char **)calloc(9, BUFFER_SIZE / 9);
    socklen_t peerInfoSize = sizeof(struct sockaddr_in);
    
    struct pollfd fds[MAX_CONNECTIONS];
    fds[0].fd = master->servSock;
    fds[0].events = POLLIN;

    //int activeConnections_current; 

    int nfds = 1;
    int peerfd;

    while (true)
    {
        if (poll(fds, nfds, -1) == -1)
        {
            printf("Failed to poll\n");
            //Some error
        }

        for (int i = 0; i < nfds; i++)
        {
            //New Connection
            if (fds[i].fd == master->servSock)
            {
                do 
                {
                    peerfd = accept(master->servSock, (struct sockaddr*)&master->atvPeers[nfds].public, &peerInfoSize);
                    //If no new connection
                    if (peerfd == -1) break;

                    //Announces the new connection
                    printf("New connection at %s:%i\n", inet_ntoa(master->atvPeers[nfds].public.sin_addr), ntohs(master->atvPeers[nfds].public.sin_port));

                    //Adds client info
                    fds[nfds].fd = peerfd;
                    fds[nfds].events = POLLIN;
                    master->atvPeers[nfds].fd = &fds[nfds].fd;
                    nfds++;
                } while (peerfd != -1);
            }
            //Message from somewhere
            else if (fds[i].fd > 0 && fds[i].revents == POLLIN)
            {
                memset(buffer, 0, BUFFER_SIZE);
                memset(pBuffer, 0, BUFFER_SIZE);
                
				if (data_read(&fds[i].fd, buffer) == -1)
				{
					connection_close(&fds[i].fd);
					break;
				}

				printf("%s", buffer);
                /*data_parse(buffer, pBuffer, 2, "-");
            	strcpy(buffer, pBuffer[1]);

                printf("%c", pBuffer[0][0]);

                
                //Handles the message
                switch (pBuffer[0][0])
                {
                case 'M':
                    //memset(pBuffer, 0, BUFFER_SIZE);
                    //data_parse(buffer, pBuffer, 2, "-");
                    printf("<%s> %s\n", master->atvPeers[i].username, buffer);
                    break;
                case 'J':
                    strcpy(master->atvPeers[i].username, buffer);
                    break;
                default:
                    printf("%s\n", buffer);
                    break;
                }*/
            }
        }
    }
    
    return NULL;
    //Do some stuff to close thread nicely
}

int main(int argc, const char *argv[])
{
    struct server master;
    master.servSock = socket(AF_INET, SOCK_STREAM, 0);
    master.info.sin_family = AF_INET;
    inet_aton("127.0.0.1", &master.info.sin_addr);

    //Default Port
    unsigned short serverPort = DEFAULT_SERVER_PORT;
	unsigned short clientPort = DEFAULT_SERVER_PORT;
    char username[21] = DEFAULT_USERNAME; 

    for (int i = 1; i < argc; i++)
    {
        switch(argv[i][0])
        {
        case '-':
            switch(argv[i][1])
            {
		    case 's':   //The port this server will run on
				i++;
                serverPort = atoi(argv[i]);
                if (serverPort < 1023 || argv[i][0] == '-')
                {
                    //Some error
                }
                master.info.sin_port = htons(serverPort);
                break;
			case 'c':	//The port the client will listen on (REMOVE THIS)
				i++;
                clientPort = atoi(argv[i]);
                if (clientPort < 1023 || argv[i][0] == '-')
                {
                    //Some error
                }
				break;
            case 'u':   //Username
				i++;
                snprintf(username, 21, "%s", argv[i]);
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }

	printf("%s\n", username);

    //Lets socket to accept multiple connections and be reused 
    int on = true;  
    if (setsockopt(master.servSock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == -1)
    {
        printf("Error trying to set opt\n");
        exit(-1);       
    } 

    //Sets the server socket to be nonblocking 
    if (ioctl(master.servSock, FIONBIO, (char *)&on))
    {
        printf("ioctl\n");
        exit(-1);
    }

    // Binds serverInfo sin_addr
    if (bind(master.servSock, (struct sockaddr*)&master.info, sizeof(master.info)) != 0)
    {
        printf("Error trying to bind socket\n");
        exit(-1);
    }

    //Listen
    if (listen(master.servSock, MAX_CONNECTIONS) != 0)
    {
        printf("Error trying to listen\n");
        exit(-1);
    }

    //Make a signal be sent when thread is ready
    pthread_t ioThread_id;
    pthread_create(&ioThread_id, NULL, inbound_io, &master);

    printf("Server Started! Accepting inbound connections\n");

    printf("Looking for server. . .\n");

    //This is just for now untill I can figure out a better way to handle multiple peers
    int userSock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in outboundContection;
    outboundContection.sin_family = AF_INET;
    outboundContection.sin_port = htons(clientPort);
    inet_aton("127.0.0.1", &outboundContection.sin_addr);

    //Attempts to connect to socket
    int connectionState;
    do 
    {
        connectionState = connect(userSock, (struct sockaddr*)&outboundContection, sizeof(outboundContection));
        if (connectionState == -1) sleep(1);
    } while (connectionState == -1);

    printf("Connected to peer at %s:%i\n", inet_ntoa(outboundContection.sin_addr), ntohs(outboundContection.sin_port));

    char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
    char *msgBuffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        memset(msgBuffer, 0, BUFFER_SIZE);
        scanf("%99[^\n]", buffer);
        snprintf(msgBuffer, BUFFER_SIZE, "%s", buffer);
        send(userSock, msgBuffer, BUFFER_SIZE, 0);
    }
}