#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

void *handleConnection(void *ptr)
{
    int sock = *((int *)ptr);
    printf("New connection accepted: %d\n", sock);
    char client_message[200] = {0};
    char pMessage[200] = "Hello";
    char message[200] = "Hey!";
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
            if (strcmp(client_message, "quit") == 0) {
                printf("Quitting: %d\n", sock);
                break;
            }
            printf("Client reply (%d) : %s\n", read_size, client_message);
        }

        // strcpy(message, "Message from server!");
        // // Send some data
        // if (send(sock, message, strlen(message), 0) < 0)
        // {
        //     printf("Send failed");
        //     return NULL;
        // }
        sleep(1);
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
        printf("Waiting for incoming connections...\n");
        clientLen = sizeof(struct sockaddr_in);
        // accept connection from an incoming client
        sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&clientLen);
        int* socketid = (int*)malloc(sizeof(int));
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