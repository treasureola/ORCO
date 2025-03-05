#include "comm.h"

/*
 * handle_client
 *
 * This function processes a client connection and populates the client's
 * file descriptors given its credentials
 */
void handle_client(struct ucred *creds, int client_fd)
{
    pid_t pid = creds->pid;
    uid_t uid = creds->uid;
    gid_t gid = creds->gid;
    (void)pid;
    (void)uid;
    (void)gid;
    (void)client_fd;
}

/*
 * verify_client
 *
 * This function verifies a client's credentials
 */
void verify_client(int client_fd)
{
    DEBUG_PRINT("Server: Client %d connected\n", client_fd);

    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred)) + CMSG_SPACE(sizeof(int));
    char *controlMsg = malloc(controlMsgSize);

    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    char dummy;
    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;

    ssize_t bytesReceived = recvmsg(client_fd, &msgh, 0);
    if (bytesReceived < 0 || bytesReceived == 0)
    {
        perror("Failed to receive credentials");
        exit(EXIT_FAILURE);
    }

    struct cmsghdr *cmsgp;
    struct ucred *creds;
    for (cmsgp = CMSG_FIRSTHDR(&msgh); cmsgp != NULL; cmsgp = CMSG_NXTHDR(&msgh, cmsgp))
    {
        switch (cmsgp->cmsg_type)
        {
        case SCM_CREDENTIALS:
            creds = (struct ucred *)CMSG_DATA(cmsgp);
            DEBUG_PRINT("Received pid: %d\n", creds->pid);
            DEBUG_PRINT("Received uid: %d\n", creds->uid);
            DEBUG_PRINT("Received gid: %d\n", creds->gid);
            break;
        case SCM_RIGHTS:
            int fdCnt = (cmsgp->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            int *fdTable = (int *)CMSG_DATA(cmsgp);
            for (int i = 0; i < fdCnt; i++)
            {
                DEBUG_PRINT("Received fd: %d\n", fdTable[i]);
                // Duplicate the file descriptor
                int newFd = dup(fdTable[i]);
                if (newFd == -1)
                {
                    perror("dup");
                    exit(EXIT_FAILURE);
                }

                // Send message
                DEBUG_PRINT("file descriptor: %d\n", fdTable[i]);
                DEBUG_PRINT("Sending message to client\n");
                char *msg = "Hello from server";
                ssize_t bytesWritten = write(fdTable[i], msg, strlen(msg));
                if (bytesWritten == -1)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            break;
        default:
            break;
        }
    }

    handle_client(creds, client_fd);

    free(controlMsg);
    close(client_fd);
}

/*
 * Server program
 * 
 *  This file simulates a server program that listens for incoming client
 *  connections over a named UNIX socket. It receives a client's credentials
 *  and files descriptors and prcoesses them.
 */
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

    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    // Listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Set up the poll structure for the listening socket
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN; // Monitor incoming connections (read events)

    DEBUG_PRINT("Server listening\n");

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

                    // Handle client verification
                    verify_client(client_fd);

                    // Add the new client socket to the poll structure
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN; // Monitor client for incoming data
                    nfds++;  // Increase the number of file descriptors being monitored
                    printf("%s %d\n", "NEW CLIENT ACCEPTED", client_fd);

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