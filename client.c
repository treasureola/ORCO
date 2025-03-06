#include "comm.h"

// TODO 
    // Write a function that handles sserver request where after client 
/*
 * named_connect
 *
 * This function connects to a named UNIX socket with credential
 * and file descriptor passing enabled.
 */
int named_connect(const char *socket_path){
    // Create the named socket connection
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;                                      // UNIX domain socket
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1); // Set the path

    // Connect to the server
    int status = connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (status < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Enable passing of credentials
    int enable = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_PASSCRED, &enable, sizeof(enable));

    return socket_fd;
}

/*
 * Client program
 *
 * This file simulates a client program that connects to the server
 * over a named UNIX socket. It sends its credentials and file descriptors
 * across the socket to the server. 
 */

void populate_request(int fd, int access_code, char *action, char *filename, char *write_string, int *read_bytes){
    Request request;
    // Request request = {access_code, action, filename, write_string, read_bytes};
    request.acces_code = access_code;  //Assign the user access code input
    request.action = action; // Assign the User action (read or write)
    request.filename = filename; //Assign the user filename
    request.write_string = write_string;
    request.read_bytes = read_bytes;
    //Write to the fd
    // write(fd, &request, sizeof(Request));
}

int main(){
    // Initialize client data
    pid_t pid = getpid();
    uid_t uid = getuid();
    uid = 1000;
    gid_t gid = getgid();
    gid = 1000;
    
    DEBUG_PRINT("Client pid: %d\n", pid);
    DEBUG_PRINT("Client uid: %d\n", uid);
    DEBUG_PRINT("Client gid: %d\n", gid);

    // Connect to the server
    int socket_fd = named_connect(SOCKET_PATH);
    
    // Define the credentials
    struct ucred creds;
    creds.pid = pid;
    creds.uid = uid;
    creds.gid = gid;

    // Prepare the message
    struct msghdr msgh;
    memset(&msgh, 0, sizeof(struct msghdr));

    // Set up the message
    struct iovec iov;
    char dummy = 0; // Dummy byte to satisfy `msg_iov`

    iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;

    // Allocate control message buffer
    size_t fdTableSize = sizeof(int);
    size_t controlMsgSize = CMSG_SPACE(sizeof(struct ucred)) + CMSG_SPACE(fdTableSize);
    char *controlMsg = malloc(controlMsgSize);
    if (!controlMsg)
    {
        perror("Memory allocation failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    memset(controlMsg, 0, controlMsgSize);

    // Set up the control message
    msgh.msg_control = controlMsg;
    msgh.msg_controllen = controlMsgSize;
    
    // Populate the first header (credentials)
    struct cmsghdr *cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_CREDENTIALS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(struct ucred));

    // Copy the credentials into the control message
    memcpy(CMSG_DATA(cmsgp), &creds, sizeof(struct ucred));

    // // Populate the second header (file descriptors)
    cmsgp = CMSG_NXTHDR(&msgh, cmsgp);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(fdTableSize);

    // Open the file to send
    int *fdTable = malloc(fdTableSize);
    fdTable[0] = open("client.txt", O_CREAT | O_RDWR);
    if (fdTable[0] < 0) {
        perror("Failed to open file");
        close(socket_fd);
        free(fdTable);
        exit(EXIT_FAILURE);
    }

    // // Copy the file descriptors into the control message
    memcpy(CMSG_DATA(cmsgp), fdTable, fdTableSize);

    // Send the message
    ssize_t bytesSent = sendmsg(socket_fd, &msgh, 0);
    if (bytesSent < 0) {
        perror("Failed to send credentials");
        exit(EXIT_FAILURE);
    }

    close(fdTable[0]);

    DEBUG_PRINT("Sending message to server\n");

    char message[] = "Hello from client!\n";
    int sent_byte = send(socket_fd, message, strlen(message), 0);

    if (sent_byte < 0) {

        perror("Send failed");

        exit(1);

    }

    DEBUG_PRINT("Message sent to server\n");

    // Now wait for a response from the server
    char buffer[1024] = {0};
    ssize_t response_len = read(socket_fd, buffer, sizeof(buffer) - 1);  // Use read to block and receive data
    if (response_len < 0) {
        perror("Failed to receive response from server");
        exit(EXIT_FAILURE);
    } else if (response_len == 0) {
        printf("Server closed the connection\n");
    } else {
        buffer[response_len] = '\0';  // Null-terminate the received data
        printf("Received response from server: %s\n", buffer);
    }
    
    // Close the socket
    close(socket_fd);

    free(controlMsg);

    return 0;
}