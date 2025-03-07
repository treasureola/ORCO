# Advanced OS Coding Project I - UNIX Service

## Team ORCO

### Table of contents

- [Advanced OS Coding Project I - UNIX Service](#advanced-os-coding-project-i---unix-service)
  - [Team ORCO](#team-orco)
    - [Table of contents](#table-of-contents)
    - [Overview](#overview)
      - [The Database](#the-database)
      - [Client Request Function](#client-request-function)
      - [Access Control Logic](#access-control-logic)
        - [Access Level Check](#access-level-check)
        - [Handling Actions](#handling-actions)
        - [Error Handling](#error-handling)
      - [Server Behavior](#server-behavior)
        - [Authenticate the Client](#authenticate-the-client)
        - [Process the Request](#process-the-request)
      - [Example Workflow](#example-workflow)
    - [Project Goals](#project-goals)
      - [1 - Named Socket Communication](#1---named-socket-communication)
      - [2 - Connection-per-client](#2---connection-per-client)
      - [3 - Event Notifcation and Concurrency](#3---event-notifcation-and-concurrency)
      - [4 - Client Certification Through Sockets](#4---client-certification-through-sockets)
      - [5 - File Descriptor Passing](#5---file-descriptor-passing)
    - [Work Distribution](#work-distribution)
      - [Oliver](#oliver)
      - [Fred](#fred)
      - [Treasure](#treasure)
    - [Testing](#testing)

---

### Overview

Our project aims to create a UNIX daemon that manages concurrent database requests through domain sockets. The service will allow clients to interact with specific files in the database, depending on their access privileges.

#### The Database

We have 3 files within the repo that emulates the functionality of our pseudo database: `file1.txt`, `file2.txt`, and `file3.txt`. The access levels are assigned as follows:

- **Access level 1:** Grants a client *read and write* access only to `file1.txt`.
- **Access level 2:** Grants a client *read and write* access to both `file1.txt` and `file2.txt`.
- **Access level 3:** Grants a client *read and write* access to all three files: `file1.txt`, `file2.txt`, and `file3.txt`.

#### Client Request Function

To interact with the server and the database, the client uses the `populate_request` function to send the specifics of their accress request. The function prepares a request struct and sends it over a socket to the server, where the request is then processed.

> **comm.h**

```c
void populate_request(int fd, int access_code, char * action, char* filename, char * write_string, int read_bytes)
```

The function above sends the request in the form of a **Request** struct:

> **comm.h**

```c
typedef struct {
    int acces_code; //the access code of the user
    char action[50]; // To read or write
    char filename[128];  // file the user wnats to read or wort to
    char write_string[1024]; // IF write, the data the user wants to worite to the file
    int read_bytes; //If read, the number of bytes the user wants to read
}Request;
```

#### Access Control Logic

After a client has successfully communicated a database request, the server will receive the message and verify whether the given client has the required access level for the file they are trying to interact with. The access control logic ensures that the client can only perform actions (read/write) on files they are permitted to access based on their access level, as seen below in `handle_request()`:

> **server.c - handle_request()**

**Write Request**

```c
if (strcmp(request.action, "Write") == 0){
    DEBUG_PRINT("Recieved write command\n");
    FILE *file = fopen(request.filename, "w");
    fprintf(file, "%s\n",request.write_string);
    char *ret = "DONE: DATA HAS BEEN WRITTEN INTO THE FILE";
    send(fd, ret, strlen(ret), 0);
    fclose(file);  
    return; 
```

---

**Read Request**

```c
else {
    DEBUG_PRINT("Recieved read command\n");
    int file_fd = open(request.filename, O_RDONLY);
    char buffer[request.read_bytes + 1];
    size_t bytes_read = read(file_fd, &buffer, request.read_bytes);
    buffer[bytes_read] = '\0'; // Null-terminate for printing as a string
    send(fd, buffer, strlen(buffer), 0);
    close(file_fd);
    return;
} 
```

---

**Failure**

```c
char *ret = "ACCESS DEINED\n";
send(fd, ret, strlen(ret), 0);
return;
```

---

##### Access Level Check

- For each request, the server will check the client’s access_code.
- The server compares the requested file (filename) with the files that the access level allows the client to access.
- If the access code does not permit access to the requested file, the server will return an error.

##### Handling Actions

- If the action is read, the server will check if the requested file exists and then allow reading the specified number of bytes (read_bytes).
- If the action is write, the server will check if the client is authorized to write to the specified file (filename). If authorized, the server will write the data from write_string to the file.

##### Error Handling

- If the access level is insufficient, the server will reject the request and return an error message.
- If the action is not supported or the file is not found, the server will also return an error.

#### Server Behavior

The server will host the database files and listen for incoming client connections on a specified named UNIX socket. Upon receiving a request, the server will:

##### Authenticate the Client

- Once a client connects for the first time, its sents its credential to the server to be verfied. If the verification fails, the client will be disconnected.

> **client.c - main()**

```c
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
```

> **server.c - verify_client()**

```c
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
}
```

- Once the client credential has been verified it can now send requests to the server. The server then checks the provided **access_code** against the client's privileges and determine what files the client can access.

##### Process the Request

- If the request is a read action, the server will read the specified number of bytes from the requested file.
- If the request is a write action, the server will write the provided data (write_string) to the requested file.
- Return the Result: The server will send a success or error message back to the client, based on the outcome of the request.

#### Example Workflow

- A client with access level 2 connects to the server and sends a request to read from **file1.txt**.
- The server checks if the client's **access_code (2)** allows access to **file1.txt**. Since access level 2 includes **file1.txt**, the request is valid.
- The client sends a request to read 100 bytes from **file1.txt**. The server reads the data and sends it back to the client.
- If the client with access level 2 tries to access **file3.txt**, the server will reject the request with an error message because access level 2 does not allow access to **file3.txt**.

---

### Project Goals

Our project hopes to create a UNIX service daemon that hosts a pseudo database and manages access control to different parts of the database based on client access levels.

To facilitate this communication and control flow, our project closely follows five main goals/structures to enable client database interaction and server management. Below is a list of the five goals listed in the project spec and how our system design aims to address each:

#### 1 - Named Socket Communication

> Use UNIX domain sockets for communication with a known path for the name of the socket.

To make request to the server, each client must first connect to the server and be accepted in order to communicate to the database. In order to connect to the server, clients create socket connections to a **named domain socket**, which is binded to an address. Clients can connect to `/tmp/socket` in order to begin communication with the server and be accepted.

#### 2 - Connection-per-client

> Use accept to create a connection-per-client.

Once a client connects via the named socket, the sever will use `accept()` to create a new socket file descriptor per client to facilitate individual request handling on private sockets instead of over the main named socket.

#### 3 - Event Notifcation and Concurrency

> Use event notification (epoll, poll, select, etc...) to manage concurrency

In order to service all client request, the server employs an **event loop** that continuously checks for client requests over the newly created file descriptors (via `accept()`). While waiting, the server uses `poll()` to keep itself in a busy loop and only continue when an incoming request has been detected across the server's **per-client socket file descriptors**.

#### 4 - Client Certification Through Sockets

> Use domain socket facilities to get a trustworthy identity of the client (i.e. user id).

When clients initially connect and wait to be accepted by the server, they must communicate and present their client certification through `SCM_CREDENTIALS`. Once recieved, the server will verify an incoming credentail list, including the processes user level and group, and ensure that the credentials were properly communicated over the domain socket.

#### 5 - File Descriptor Passing

> Pass file descriptors between the service and the client.

---

### Work Distribution

This section outlines the work distribution and contribution of each member of team ORCO.

#### Oliver

- Named Socket Communication
  - Server can open a named socket with correct operations for communication with incoming clients
    - Server binds socket to fs
    - Accepts incoming connection requests from client
  - Clients can open same named socket to send request over the server
- Client Certification
  - Ensure that a client can communicate their personal identification over an opened named socekt
  - Correctly recieve message from connected client

#### Fred

- Event loop
  - Server can recieve incoming client connections through the name socket and save the client socket fd table
  - Server can identify when any existing client already in the fd table is sending a request through POLLIN
- Database
  - Contributed to the development of the database concept for the project and assisted in its implementation
- Test
  - Tested the core functionality of the server, including handling multiple client connections and scenarios such as when a client fails to send credentials
- Documentation:
  - Overview and Project Goals

#### Treasure

- Event Loop
  - Server can recieve incoming client connections through the name socket and save the client socket fd table
  - Server can identify when any existing client already in the fd table is sending a request through POLLIN
- Database
  - Contributed to the development of the database concept for the project and assisted in its implementation

---

### Testing
