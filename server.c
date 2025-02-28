#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <poll.h>  // For using poll instead of epoll

#define SOCKET_PATH "/tmp/socket"
#define MAX_EVENTS 10 // Maximum number for poll to handle at once

struct ucred
{
    pid_t pid;
    uid_t uid;
    gid_t gid;
};

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

    int socket_fd; // fd for socket
    struct pollfd fds[MAX_EVENTS]; // fds: array to store pollfd structures
    int nfds = 1, client_fd; // nfds: number of fds ready for processing, client_fd: new client connection

    // Create the socket
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

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

    // Set up the poll structure for the listening socket
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN; // Monitor incoming connections (read events)

    printf("Server listening\n");

    // Event Loop using poll
    while (1)
    {
        // Wait for events on the monitored file descriptors
        int ret = poll(fds, nfds, -1);  // Wait indefinitely for events
        if (ret == -1) {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                // If it's the listening socket, accept a new client
                if (fds[i].fd == socket_fd) {
                    client_fd = accept(socket_fd, NULL, NULL);
                    if (client_fd == -1) {
                        perror("accept");
                        continue;
                    }

                    // Handle client authentication
                    handle_client(client_fd);

                    // Add the new client socket to the poll structure
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN; // Monitor client for incoming data
                    nfds++;  // Increase the number of file descriptors being monitored

                } else {
                    // If it's an existing client, read data
                    char buf[256];
                    int r = read(fds[i].fd, buf, sizeof(buf) - 1);

                    if (r > 0) {
                        buf[r] = '\0';  // Null-terminate string
                        printf("Received: %s\n", buf);
                        if (write(fds[i].fd, buf, r) == -1) {
                            perror("write");
                        }
                    } else {
                        if (r == 0) {
                            printf("Client disconnected\n");
                        } else {
                            perror("read");
                        }

                        // Remove client from the poll set
                        close(fds[i].fd);

                        // Shift the remaining fds to fill the gap
                        for (int j = i; j < nfds - 1; j++) {
                            fds[j] = fds[j + 1];
                        }
                        nfds--;  // Decrease the number of file descriptors being monitored
                        i--;  // Adjust the index to check the shifted client
                    }
                }
            }
        }
    }
    close(socket_fd);
    unlink(SOCKET_PATH);  // Clean up the socket file when done
    return 0;
}