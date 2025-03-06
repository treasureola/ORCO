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

void handle_request(Request request, int fd){
    int files[3] = {"file1.txt", "file2.txt", "file3.txt"};
    for (int i=0; i<=request.acces_code; i++){
        if (request.filename == files[i]){
            if (request.action == "Write"){ //if the 
                FILE *file = fopen(request.filename, "w");
                fprintf(file, request.write_string);
                char *ret = "DONE: DATA HAS BEEN WRITTEN INTO THE FILE";
                send(fd, ret, strlen(ret), 0);
                fclose(file);
            } else {
                FILE *file = fopen(request.filename, "r");
                char buffer[request.read_bytes + 1];
                size_t bytes_read = (buffer, 1, request.read_bytes, file);
                buffer[bytes_read] = '\0'; // Null-terminate for printing as a string
                send(fd, buffer, strlen(buffer), 0);
                fclose(file);
            }
        } else {
            perror("ACCESS TO FILE DENIED");
        }
    }
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
            }
            break;
        default:
            break;
        }
    }

    // handle_client(creds, client_fd);

    free(controlMsg);
    // close(client_fd);
    return;
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
    memset(fds, -1, sizeof(fds));
    int nfds = 1, client_fd; // nfds: number of fds ready for processing, client_fd: new client connection
    char buffer[1024] = {0};

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
    while (1) {
        // Wait for events on the monitored file descriptors
        int ret = poll(fds, nfds, -1);  // Wait indefinitely for events
        if (ret == -1) {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        // New connection
        if (fds[0].revents & POLLIN) {
            if ((client_fd = accept(socket_fd, NULL, NULL)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            printf("New connection, socket fd is %d\n", client_fd);

            // Add new socket to pollfd array
            for (int i = 1; i <= MAX_CLIENTS; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    break;
                }
            }
            verify_client(client_fd);
            DEBUG_PRINT("Cleint fd: %d accepted\n", client_fd);
        }

        // Handle client connections
        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd > 0 && fds[i].revents & POLLIN) {
                int valread = read(fds[i].fd, buffer, 1024);
                if (valread == 0) {
                    // Client disconnected
                    printf("Client disconnected, socket fd is %d\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else if (valread < 0) {
                    perror("read failed");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    // Process client request
                    Request request;
                    // buffer[valread] = '\0';
                    printf("Received from client %d: %s\n", fds[i].fd, buffer);
                    int bytes_read = recv(fds[i].fd, &request, sizeof(Request), 0);
                    if (bytes_read < 0){
                        perror("Send failed");
                        exit(1);
                    }
                    handle_request(request, fds[i].fd);
                    // send(fds[i].fd, buffer, strlen(buffer), 0);
                    close(fds[i].fd);
                }
            }
        }
    }
    close(socket_fd);
    unlink(SOCKET_PATH);  // Clean up the socket file when done
    return 0;
}