#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <sys/types.h> //getaddrinfo
#include <netdb.h>	   //getaddrinfo
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>

#include "../includes/log.h"

#define BUFFER_SIZE 9999
#define MAX_CONNECTIONS 1024

#define DEFAULT_USERNAME "Anonymous"
#define DEFAULT_SERVER_PORT 7168
#define CLIENT_PORT 7169

static pthread_mutex_t fdChange = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sigIO = PTHREAD_COND_INITIALIZER;
static char *ioBuffer;
static bool debugMode = false;

// Everything is a peer and it hold important infomation about the session
struct peer
{
	int *fd;
	// Max length of username is 20 + \0
	char username[21];
	int publicKey;

	struct sockaddr_in private;
	struct sockaddr_in public;
};

// This is shared between all threads
struct server
{
	int inboundSock;
	int outboundSock;
	// int totalActiveConnections;
	struct peer atvPeers[MAX_CONNECTIONS];

	struct sockaddr_in info;
};

int compress_array(struct peer *fds[], int length)
{
	return 0;
}

// Gets the ip that the client is using to connect to server - Fix: gethostbyname() is deprecated
char *get_private_ip()
{
	// Gets host infomation
	char hostNameBuf[256];
	struct hostent *hostInfo;

	gethostname(hostNameBuf, sizeof(hostNameBuf));
	hostInfo = gethostbyname(hostNameBuf); // DEPRECATED

	// This converts the ip address to ASCII
	return (inet_ntoa(*((struct in_addr *)hostInfo->h_addr_list[0])));
	// return (struct in_addr*)hostInfo->h_addr_list[0];
}

//

int connection_close(int *fd)
{
	struct sockaddr_in clientInfo;
	socklen_t clientInfo_size = sizeof(clientInfo);

	getpeername(*fd, (struct sockaddr *)&clientInfo, &clientInfo_size);
	printf("Connection lost at %s:%i\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
	close(*fd);
	*fd = -1;

	return 0;
}

// Accepts ALL new connections at fd
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

	// If message failed to read
	// connection_close(fd);
	return -1;
}

//
int data_write(char *message, int *fd)
{
	// Encrpyt message

	// Create http request

	// Send request to fd
	write(*fd, message, BUFFER_SIZE);

	return 0;
}

//

// Polls for events
void *inbound_io(void *master_origin)
{
	struct server *master = (struct server *)master_origin;

	ioBuffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
	socklen_t peerInfoSize = sizeof(struct sockaddr_in);

	struct pollfd fds[MAX_CONNECTIONS] = {0};
	fds[0].fd = master->inboundSock;
	fds[0].events = POLLIN;

	int nfds = 1, currentnfds = 1;
	int peerfd;

	while (true)
	{
		if (poll(fds, nfds, 100000) == -1)
		{
			printf("Failed to poll\n");
			// Some error
		}

		currentnfds = nfds;
		for (int i = 0; i < currentnfds; i++)
		{
			// New Connection
			if (fds[i].fd == master->inboundSock)
			{
				do
				{
					peerfd = accept(master->inboundSock, (struct sockaddr *)&master->atvPeers[nfds].public, &peerInfoSize);
					// If no new connection
					if (peerfd == -1)
					{
						break;
					}

					// Adds client info
					fds[nfds].fd = peerfd;
					fds[nfds].events = POLLIN;
					master->atvPeers[nfds].fd = &fds[nfds].fd;

					// Announces the new connection
					printf("New connection at %s:%i\n", inet_ntoa(master->atvPeers[nfds].public.sin_addr), ntohs(master->atvPeers[nfds].public.sin_port));

					nfds++;
				} while (peerfd != -1);
			}
			// Message from somewhere
			else if (fds[i].fd > 0 && fds[i].revents == POLLIN)
			{
				memset(ioBuffer, 0, BUFFER_SIZE);

				if (data_read(&fds[i].fd, ioBuffer) == -1)
				{
					connection_close(&fds[i].fd);
					break;
				}
				pthread_cond_broadcast(&sigIO);
				printf("%s", ioBuffer);
			}
		}
	}

	return NULL;
	// Do some stuff to close thread nicely
}

int main(int argc, const char *argv[])
{
	struct server master;
	master.inboundSock = socket(AF_INET, SOCK_STREAM, 0);
	master.info.sin_family = AF_INET;
	inet_aton("127.0.0.1", &master.info.sin_addr);

	// Default Port
	unsigned short serverPort = DEFAULT_SERVER_PORT;
	unsigned short clientPort = DEFAULT_SERVER_PORT;
	char username[21] = DEFAULT_USERNAME;

	for (int i = 1; i < argc; i++)
	{
		switch (argv[i][0])
		{
		case '-':
			switch (argv[i][1])
			{
			case 's': // The port this server will run on
				i++;
				serverPort = atoi(argv[i]);
				if (serverPort < 1023 || argv[i][0] == '-')
				{
					// Some error
				}
				master.info.sin_port = htons(serverPort);
				break;
			case 'c': // The port the client will listen on (REMOVE THIS)
				i++;
				clientPort = atoi(argv[i]);
				if (clientPort < 1023 || argv[i][0] == '-')
				{
					// Some error
				}
				break;
			case 'u': // Username
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

	// Lets socket to accept multiple connections and be reused
	int on = true;
	if (setsockopt(master.inboundSock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == -1)
	{
		printf("Error trying to set opt\n");
		exit(-1);
	}

	// Sets the server socket to be nonblocking
	if (ioctl(master.inboundSock, FIONBIO, (char *)&on))
	{
		printf("ioctl\n");
		exit(-1);
	}

	// Binds serverInfo sin_addr
	if (bind(master.inboundSock, (struct sockaddr *)&master.info, sizeof(master.info)) != 0)
	{
		printf("Error trying to bind socket\n");
		exit(-1);
	}

	// Listen
	if (listen(master.inboundSock, MAX_CONNECTIONS) != 0)
	{
		printf("Error trying to listen\n");
		exit(-1);
	}

	// Make a signal be sent when thread is ready
	pthread_t ioThread_id;
	pthread_create(&ioThread_id, NULL, inbound_io, &master);

	printf("Server Started! Accepting inbound connections\n");

	char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
	char *tempBuffer[2];

	int userSock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in outboundContection;

	while (true)
	{
		memset(buffer, 0, BUFFER_SIZE);
		scanf("%99[^\n]", buffer);
		if (buffer[0] == '/')
		{
			buffer++;
			data_parse(buffer, tempBuffer, 2, ":");
			printf("Attempting to connect to client\n");
			outboundContection.sin_family = AF_INET;
			outboundContection.sin_port = htons(atoi(tempBuffer[1]));
			inet_aton("127.0.0.1", &outboundContection.sin_addr);
			int connectionState;
			do
			{
				connectionState = connect(userSock, (struct sockaddr *)&outboundContection, sizeof(outboundContection));
				if (connectionState == -1)
					sleep(1);
			} while (connectionState == -1);
			printf("Connected!\n");
		}
		else
		{
			send(userSock, buffer, BUFFER_SIZE, 0);
		}
	}

	/*printf("Looking for server. . .\n");

	// This is just for now untill I can figure out a better way to handle multiple peers
	int userSock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in outboundContection;
	outboundContection.sin_family = AF_INET;
	outboundContection.sin_port = htons(clientPort);
	inet_aton("127.0.0.1", &outboundContection.sin_addr);

	// Attempts to connect to socket
	int connectionState;
	do
	{
		connectionState = connect(userSock, (struct sockaddr *)&outboundContection, sizeof(outboundContection));
		if (connectionState == -1)
			sleep(1);
	} while (connectionState == -1);

	printf("Connected to peer at %s:%i\n", inet_ntoa(outboundContection.sin_addr), ntohs(outboundContection.sin_port));

	char *msgBuffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
	while (true)
	{
		memset(buffer, 0, BUFFER_SIZE);
		memset(msgBuffer, 0, BUFFER_SIZE);
		scanf("%99[^\n]", buffer);
		snprintf(msgBuffer, BUFFER_SIZE, "%s", buffer);
		send(userSock, msgBuffer, BUFFER_SIZE, 0);
	}*/
}