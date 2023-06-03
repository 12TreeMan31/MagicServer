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

/*#define WARNING(...) fprintf(stderr)
#define ERROR(msg) fprintf(stderr, msg) exit(0);
#define PANIC(...) perror(__VA_ARGS__) exit(0)*/

#define MAX_CLIENTS 1024

void PANIC(const char *message) { perror(message); exit(0); };
void ERROR(const char *message) { printf("%s\n", message); exit(0); };
void WARNING(const char *message) { printf("%s\n", message); };

struct Client
{
    int fd;
    char *username;
    int publicKey;
    bool waiting;

    struct sockaddr_in private;
    struct sockaddr_in public;
};

struct Server
{
    int socket;
    struct sockaddr_in serverInfo;

    struct Client *activeClients;
};

struct Party
{
    char* partyName;
    char* password;

    struct Client *partyMembers;
};

static struct Server *serverMain;


void argumentCheck(int argc, char *argv[])
{
    if (argc > 1)
    {
        serverMain = malloc(sizeof(*serverMain));           //Allocates the main server stuff

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
                            serverMain->serverInfo.sin_port = htons(port);
                            break;
                        
                        case 'a':       //IP address

                            break;
                        
                        case 's':       //Server mode

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
    else
    {
        ERROR("Insufficient arguments");
    }
} 

//Server 
void serverStart()
{
    serverMain->socket = socket(AF_INET, SOCK_STREAM, 0);
    serverMain->serverInfo.sin_family = AF_INET;

    //Configures socket options 
    int opt = true;  
    if (setsockopt(serverMain->socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1)
    { 
        PANIC("Could not configure socket"); 
    }

    // Binds serverInfo sin_addr
    if (bind(serverMain->socket, (struct sockaddr*)&serverMain->serverInfo, sizeof(serverMain->serverInfo)) == -1)
    {
        PANIC("Failed to bind socket");
    }

    listen(serverMain->socket, 1024);
    printf("Server Started!\n");
}

//Client
void attemptConnection(int *clientSocket, struct sockaddr_in serverInfo)
{
    int connectionState;
    do
    {
        connectionState = connect(*clientSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo));
    } while (connectionState == -1);
}


int main(int argc, char *argv[])
{
    argumentCheck(argc, argv);
}