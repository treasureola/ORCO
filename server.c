#include "comm.h"

void handle_client(int client_fd)
{
    printf("Client %d connected\n", client_fd);

    struct ucred creds;
    socklen_t len = sizeof(creds);
    if (getsockopt(client_fd, SOL_SOCKET, LOCAL_PEERCRED, &creds, &len) < 0)
    {
        perror("getsockopt failed");
    }
    else
    {
        printf("Client UID: %d, GID: %d, PID: %d\n", creds.uid, creds.gid, creds.pid);
    }
}

int main() {

    // Create the socket
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    unlink(SOCKET_PATH);

    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    memset(addr, 0, sizeof(struct sockaddr_un));

    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, SOCKET_PATH, sizeof(addr->sun_path) - 1);

    unlink(SOCKET_PATH);
    // Bind the socket
    if (bind(socket_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening\n");
    while (1)
    {
        // Accept the incoming connection
        int client_fd = accept(socket_fd, NULL, NULL);
        if (client_fd < 0)
        {
            continue;
        }

        // Handle the connection in a separate function or thread
        handle_client(client_fd);
    }
}