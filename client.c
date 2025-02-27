#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
    // Create the socket
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    memset(addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, "/tmp/socket", sizeof(addr->sun_path) - 1);

    printf("Client connecting\n");
    // Connect to the server
    int status = connect(socket_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_un));
    if (status < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server\n");

    // // Send a message to the server
    // char *message = "Hello, server!";
    // send(socket_fd, message, strlen(message), 0);

    // // Receive a message from the server
    // char buffer[1024];
    // recv(socket_fd, buffer, sizeof(buffer), 0);
    // printf("Received: %s\n", buffer);

    return 0;
}