#include "comm.h"

/*
 * Server program
 * 
 *  This file simulates a server program that listens for incoming client
 *  connections over a named UNIX socket. It receives a client's credentials
 *  and files descriptors and prcoesses them.
 */

void handle_request(Request request, int fd){
    char *files[3] = {"file1.txt", "file2.txt", "file3.txt"};
    DEBUG_PRINT("In handle_request\n");
    DEBUG_PRINT("%s\n", request.filename);
    for (int i=0; i<request.acces_code; i++){
        if (strcmp(request.filename , files[i])==0){
            if (strcmp(request.action, "Write") == 0){
                DEBUG_PRINT("Recieved write command\n");
                FILE *file = fopen(request.filename, "w");
                fprintf(file, "%s\n",request.write_string);
                char *ret = "DONE: DATA HAS BEEN WRITTEN INTO THE FILE";
                send(fd, ret, strlen(ret), 0);
                fclose(file);  
                return; 
            } else {
                DEBUG_PRINT("Recieved read command\n");
                int file_fd = open(request.filename, O_RDONLY);
                char buffer[request.read_bytes + 1];
                size_t bytes_read = read(file_fd, &buffer, request.read_bytes);
                buffer[bytes_read] = '\0'; // Null-terminate for printing as a string
                send(fd, buffer, strlen(buffer), 0);
                close(file_fd);
                return;
            }
        } 
    }
    char *ret = "ACCESS DEINED\n";
    send(fd, ret, strlen(ret), 0);
    return;
}

/*
 * verify_client
 *
 * This function verifies a client's credentials
 */
int verify_client(int client_fd)
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
        return FALSE;
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
                    return FALSE;
                }
            }
            break;
        default:
            break;
        }

        return TRUE;
    }

    // handle_client(creds, client_fd);

    free(controlMsg);
    // close(client_fd);
    return;
}

void setup_socket(int *socket_fd, struct sockaddr_un **addr) {
    // Create the socket
    *socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); 
    if (*socket_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    *addr = malloc(sizeof(struct sockaddr_un));
    memset(*addr, 0, sizeof(struct sockaddr_un));

    (*addr)->sun_family = AF_UNIX;
    strncpy((*addr)->sun_path, SOCKET_PATH, sizeof((*addr)->sun_path) - 1);

    // Bind the socket
    if (bind(*socket_fd, (struct sockaddr *)*addr, sizeof(struct sockaddr_un)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    setsockopt(*socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    if (listen(*socket_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

void accept_new_connection(int socket_fd, struct pollfd *fds, int *nfds) {
    int client_fd; //client_fd: new client connection

    if ((client_fd = accept(socket_fd, NULL, NULL)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    printf("New connection, socket fd is %d\n", client_fd);


    int client_idx = 0;
    // Add new socket to pollfd array
    for (int i = 1; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd == -1) {
            fds[i].fd = client_fd;
            fds[i].events = POLLIN;
            client_idx = i;
            break;
        }
    }

    if (!client_idx) {
        close(client_fd);
        DEBUG_PRINT("Client fd: %d rejected (max clients reached)\n", client_fd);
        return;
    }
    
    int success = verify_client(client_fd);
    if (!success) {
        close(client_fd);
        DEBUG_PRINT("Client fd: %d rejected (verify failed)\n", client_fd);
        fds[client_idx].fd = -1;
        return;
    }

    DEBUG_PRINT("Client fd: %d accepted\n", client_fd);
}

void handle_client_connections(struct pollfd *fds) {
    for (int i = 1; i <= MAX_CLIENTS; i++) {
        if (fds[i].fd > 0 && fds[i].revents & POLLIN) {
            Request request;
            int valread = read(fds[i].fd, &request, sizeof(Request));
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
                DEBUG_PRINT("Reading incoming request from client\n");
                DEBUG_PRINT("Reading from fd: %d\n", fds[i].fd);
                DEBUG_PRINT("read %d bytes\n", valread);
                handle_request(request, fds[i].fd);
                DEBUG_PRINT("Request handled\n");
                close(fds[i].fd);
            }
        }
    }
}

int main() {
    int socket_fd; // fd for socket
    struct sockaddr_un *addr; 
    struct pollfd fds[MAX_EVENTS]; // fds: array to store pollfd structures
    memset(fds, -1, sizeof(fds));
    int nfds = 1; // nfds: number of fds ready for processing

    setup_socket(&socket_fd, &addr);

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
            accept_new_connection(socket_fd, fds, &nfds);
        }

        // Handle client connections
        handle_client_connections(fds);
    }

    close(socket_fd);
    unlink(SOCKET_PATH); // Clean up the socket file when done
    return 0;
}