#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

// Text Colors
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_WHITE "\x1b[37m"

// Background Colors
#define ANSI_BG_BLACK "\x1b[40m"
#define ANSI_BG_RED "\x1b[41m"
#define ANSI_BG_GREEN "\x1b[42m"
#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_BLUE "\x1b[44m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_CYAN "\x1b[46m"
#define ANSI_BG_WHITE "\x1b[47m"

// Text Formatting
#define ANSI_BOLD_TEXT "\x1b[1m"
#define ANSI_UNDERLINE "\x1b[4m"
#define ANSI_BLINK "\x1b[5m"
#define ANSI_INVERSE "\x1b[7m"

// Reset Text Attributes
#define ANSI_COLOR_RESET "\x1b[0m"

typedef struct Client
{
    int socketFd;
    char *username;
} Client;

typedef enum
{
    USERNAME,
    MESSAGE,
} Parameter;

typedef enum
{
    OK,
    CLOSE
} Status;

typedef struct Request
{
    Parameter parameter;
    char *value;
} Request;

// parses the request body and returns a Request object
Request *parseBody(char *requestBody)
{
    Request *req = NULL;
    int i = 0;
    req = (Request *)malloc(sizeof(Request));

    // FILE *strStream;
    // strStream = fmemopen();
    switch (requestBody[0])
    {
    // for now assume the client will only set username
    case 'S':
        i = 0;
        char *username = (char *)malloc(30 * sizeof(char));
        while (requestBody[13 + i] != '\0')
        {
            username[i] = requestBody[13 + i];
            i++;
        }
        username[i] = '\0';
        req->value = username;
        req->parameter = USERNAME;
        printf("Username: %s\n", username);
        break;

    default:
        i = 0;
        int length = strlen(requestBody);
        char *message = (char *)malloc((length) * sizeof(char));
        while (requestBody[i] != '\0')
        {
            message[i] = requestBody[i];
            i++;
        }
        message[i] = '\0';
        req->parameter = MESSAGE;
        req->value = message;
        break;
    }
    return req;
}

Status handleConnection(void *ptr)
{
    Client *client = (Client *)ptr;
    int sock = client->socketFd;
    char requestBody[100];
    char client_message[200] = {0};
    char message[200] = "Hey!";

    // while (1)
    // {
    memset(client_message, '\0', sizeof client_message);
    memset(message, '\0', sizeof message);

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    // Receive a reply from the client
    if (select(sock + 1, &readfds, NULL, NULL, &timeout) < 0)
    {
        printf("select failed");
        return;
    }
    else if (FD_ISSET(sock, &readfds))
    {
        int read_size = recv(sock, client_message, 200, 0);
        if (read_size < 0)
        {
            printf("recv failed");
            return;
        }
        else if (read_size == 0)
        {
            printf(ANSI_COLOR_RED ANSI_BOLD_TEXT "%s (%d) left the chat." ANSI_COLOR_RESET "\n", client->username, sock);
            // printf(ANSI_BG_MAGENTA ANSI_BOLD_TEXT "%s (%d) left the chat." ANSI_COLOR_RESET "\n", client->username, sock);
            
            // Handle closure, cleanup, or exit as needed
            // Remove the client from a list of active clients
            return CLOSE;
        }
        if (strcmp(client_message, "quit") == 0)
        {
            printf("Quitting: %d\n", sock);
            // break;
        }
        Request *req = parseBody(client_message);
        if (req->parameter == USERNAME)
        {
            strcpy(client->username, req->value);
            printf(ANSI_BG_YELLOW ANSI_BOLD_TEXT "%s joined the chat." ANSI_COLOR_RESET "\n", client->username);
        }
        else if (req->parameter == MESSAGE)
        {
            // printf("===================\n");
            printf(ANSI_COLOR_BLUE ANSI_BOLD_TEXT "%s: " ANSI_COLOR_RESET, client->username);
            printf("%s", req->value);
        }
    }
    return OK;
    // strcpy(message, "Message from server!");
    // // Send some data
    // if (send(sock, message, strlen(message), 0) < 0)
    // {
    //     printf("Send failed");
    //     return NULL;
    // }
    // sleep(1);
    // }
    // close(sock);
    // printf("Client disconnected: %d\n", sock);
}
short SocketCreate(void)
{
    short hSocket;
    printf("Create the socket\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    return hSocket;
}
int BindCreatedSocket(int hSocket)
{
    int iRetval = -1;
    int ClientPort = 90190;
    struct sockaddr_in remote = {0};
    /* Internet address family */
    remote.sin_family = AF_INET;
    /* Any incoming interface */
    remote.sin_addr.s_addr = htonl(INADDR_ANY);
    remote.sin_port = htons(ClientPort); /* Local port */
    // Set SO_REUSEADDR option to ignore TIME_WAIT state of port
    int reuse = 1;
    if (setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    iRetval = bind(hSocket, (struct sockaddr *)&remote, sizeof(remote));
    return iRetval;
}

int main(int argc, char *argv[])
{
    int clientLimit = 30;
    Client *clientSockets = (Client *)malloc(clientLimit * sizeof(Client));
    for (int i = 0; i < clientLimit; i++)
    {
        clientSockets[i].socketFd = -1;
        clientSockets[i].username = (char *)malloc(30 * sizeof(char));
    }

    signal(SIGPIPE, SIG_IGN);
    int socket_desc, sock, clientLen, read_size;
    struct sockaddr_in server, client;
    char client_message[200] = {0};
    char message[100] = {0};
    const char *pMessage = "hello aticleworld.com";
    // Create socket
    socket_desc = SocketCreate();
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        return 1;
    }
    printf("Socket created\n");
    // Bind
    if (BindCreatedSocket(socket_desc) < 0)
    {
        // print the error message
        perror("bind failed.");
        return 1;
    }
    printf("bind done\n");
    // Listen
    listen(socket_desc, 1);

    printf(ANSI_COLOR_GREEN "Waiting for incoming connections..." ANSI_COLOR_RESET "\n");
    // Accept and incoming connection
    while (1)
    {
        clientLen = sizeof(struct sockaddr_in);

        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(socket_desc, &readfds);

        if (select(socket_desc + 1, &readfds, NULL, NULL, &timeout) < 0)
        {
            printf("select failed");
            return 1;
        }
        else if (FD_ISSET(socket_desc, &readfds))
        {
            // accept connection from an incoming client, non blocking
            int socketid = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&clientLen);
            if (socketid < 0)
            {
                perror("accept failed");
                return 1;
            }
            printf("Connection accepted: %d\n", socketid);
            clientSockets[socketid].socketFd = socketid;
        }
        // else
        // {
        //     printf("No new connections\n");
        // }

        // pthread_t thread;
        // pthread_create(&thread, NULL, handleConnection, (void *)socketid);
        for (int i = 0; i < clientLimit; i++)
            if (clientSockets[i].socketFd != -1)
            {
                Status status = handleConnection((void *)&clientSockets[i]);
                if (status == CLOSE)
                {
                    clientSockets[i].socketFd = -1;
                    memset(clientSockets[i].username, '\0', 30);
                }
            }
    }
    return 0;
}