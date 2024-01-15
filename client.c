#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

// Create a Socket for server communication
short SocketCreate(void)
{
    short hSocket;
    printf("Create the socket\n");
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    return hSocket;
}
// try to connect with server
int SocketConnect(int hSocket)
{
    int iRetval = -1;
    int ServerPort = 90190;
    struct sockaddr_in remote = {0};
    remote.sin_addr.s_addr = inet_addr("127.0.0.1"); // Local Host
    remote.sin_family = AF_INET;
    remote.sin_port = htons(ServerPort);
    iRetval = connect(hSocket, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
    return iRetval;
}
// Send the data to the server and set the timeout of 20 seconds
int SocketSend(int hSocket, char *Rqst, short lenRqst)
{
    int shortRetval = -1;
    struct timeval tv;
    tv.tv_sec = 20; /* 20 Secs Timeout */
    tv.tv_usec = 0;
    if (setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) < 0)
    {
        printf("Time Out\n");
        return -1;
    }
    shortRetval = send(hSocket, Rqst, lenRqst, 0);
    return shortRetval;
}
// receive the data from the server
int SocketReceive(int hSocket, char *Rsp, short RvcSize)
{
    int shortRetval = -1;
    struct timeval tv;
    tv.tv_sec = 20; /* 20 Secs Timeout */
    tv.tv_usec = 0;
    if (setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
    {
        printf("Time Out\n");
        return -1;
    }
    shortRetval = recv(hSocket, Rsp, RvcSize, 0);
    printf("Response %s\n", Rsp);
    return shortRetval;
}

// main driver program
int main(int argc, char *argv[])
{
    char* username[20];
    printf("Enter your username: ");
    scanf("%s", username);

    int hSocket, read_size;
    struct sockaddr_in server;
    char SendToServer[100] = {0};
    char server_reply[200] = {0};
    // Create socket
    hSocket = SocketCreate();
    if (hSocket == -1)
    {
        printf("Could not create socket\n");
        return 1;
    }
    printf("Socket is created\n");
    // Connect to remote server
    if (SocketConnect(hSocket) < 0)
    {
        perror("connect failed.\n");
        return 1;
    }
    printf("Sucessfully conected with server\n");
    char* requestBody[100];
    strcpy(requestBody, "SET username ");
    strcat(requestBody, username);
    send(hSocket, requestBody, strlen(requestBody), 0);
    printf("Sent username to server\n");

    // Communicate with the server
    printf("Enter the Message: ");
    while (1)
    {
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10;

        FD_ZERO(&readfds);
        FD_SET(hSocket, &readfds);
        FD_SET(0, &readfds);

        if (select(hSocket + 1, &readfds, NULL, NULL, &timeout) < 0)
        {
            printf("select failed");
            return 1;
        }
        // Receive a reply from the server
        else if (FD_ISSET(hSocket, &readfds))
        {
            read_size = SocketReceive(hSocket, server_reply, 200);
            if (read_size < 0)
            {
                printf("recv failed");
                return 1;
            }
            printf("Server Response : %s\n\n", server_reply);
        }
        // Send some data
        else if (FD_ISSET(0, &readfds))
        {
            fgets(SendToServer, 200, stdin);
            printf("Sending: %s", SendToServer);
            // Send data to the server
            SocketSend(hSocket, SendToServer, strlen(SendToServer));
        }
    }
    close(hSocket);
    shutdown(hSocket, 0);
    shutdown(hSocket, 1);
    shutdown(hSocket, 2);
    return 0;
}