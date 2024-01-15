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
    SET,
} Method;

typedef enum
{
    USERNAME,
    MESSAGE,
} Parameter;

typedef struct Request
{
    Method method;
    Parameter parameter;
    char *value;
} Request;

// parses the request body and returns a Request object
Request *parseBody(char *requestBody)
{
    Request *req = NULL;

    // FILE *strStream;
    // strStream = fmemopen();
    switch (requestBody[0])
    {
    // for now assume the client will only set username
    case 'S':
        req = (Request *)malloc(sizeof(Request));
        int i = 0;
        char *username = (char *)malloc(20 * sizeof(char));
        while (requestBody[13 + i] != '\0')
        {
            username[i] = requestBody[13 + i];
            i++;
        }
        username[i] = '\0';
        req->method = SET;
        req->value = username;
        req->parameter = USERNAME;
        printf("Username: %s\n", username);
        break;

    default:
        printf("Error parsing request body: %s\n", requestBody);
        break;
    }
    return req;
}

void *handleConnection(void *ptr)
{
    int sock = *((int *)ptr);
    char requestBody[100];
    printf("New connection accepted: %d\n", sock);
    char client_message[200] = {0};
    char pMessage[200] = "Hello";
    char message[200] = "Hey!";
    recv(sock, requestBody, 100, 0);
    Request *req = parseBody(requestBody);
    Client client;
    client.socketFd = sock;
    client.username = req->value;

    while (1)
    {
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
                printf("Connection closed by the client: %d\n", sock);
                // Handle closure, cleanup, or exit as needed
                // For example, you might remove the client from a list of active clients
                break; // Exiting the loop assuming the connection is closed
            }
            if (strcmp(client_message, "quit") == 0)
            {
                printf("Quitting: %d\n", sock);
                break;
            }
            printf(ANSI_COLOR_BLUE ANSI_BOLD_TEXT "%s (%d): " ANSI_COLOR_RESET, client.username, read_size);
            printf("%s", client_message);
        }

        // strcpy(message, "Message from server!");
        // // Send some data
        // if (send(sock, message, strlen(message), 0) < 0)
        // {
        //     printf("Send failed");
        //     return NULL;
        // }
        // sleep(1);
    }
    close(sock);
    printf("Client disconnected: %d\n", sock);
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
    // Accept and incoming connection
    while (1)
    {
        printf(ANSI_COLOR_GREEN "Waiting for incoming connections...\n" ANSI_COLOR_RESET);
        clientLen = sizeof(struct sockaddr_in);
        // accept connection from an incoming client
        sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&clientLen);
        int *socketid = (int *)malloc(sizeof(int));
        *socketid = sock;
        if (*socketid < 0)
        {
            perror("accept failed");
            return 1;
        }
        pthread_t thread;
        pthread_create(&thread, NULL, handleConnection, (void *)socketid);
    }
    return 0;
}