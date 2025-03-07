# Advanced OS Coding Project I - UNIX Service

## Team ORCO

### Table of contents

- [Advanced OS Coding Project I - UNIX Service](#advanced-os-coding-project-i---unix-service)
  - [Team ORCO](#team-orco)
    - [Table of contents](#table-of-contents)
    - [Overview](#overview)
      - [Client Request Function:](#client-request-function)
      - [Access Control Logic:](#access-control-logic)
        - [Access Level Check:](#access-level-check)
        - [Handling Actions:](#handling-actions)
        - [Error Handling:](#error-handling)
      - [Server Behavior:](#server-behavior)
        - [Authenticate the Client:](#authenticate-the-client)
        - [Process the Request:](#process-the-request)
      - [Example Workflow:](#example-workflow)
    - [Project Goals](#project-goals)
    - [Work Distribution](#work-distribution)
      - [Oliver](#oliver)
      - [Fred](#fred)
      - [Treasure](#treasure)
    - [Testing](#testing)

---

### Overview

The project involves creating a UNIX service that hosts a database and controls access to different parts of the database based on the access level provided by the client in their request. The service will allow clients to interact with specific files in the database, depending on their access privileges.

We have 3 files in the database: file1.txt, file2.txt, and file3.txt. The access levels are assigned as follows:

  - Access level 1: Grants a client read and write access only to file1.txt.
  - Access level 2: Grants a client read and write access to both file1.txt and file2.txt.
  - Access level 3: Grants a client read and write access to all three files: file1.txt, file2.txt, and file3.txt.

#### Client Request Function:

To interact with the server, the client will use the populate_request function to send their request. The function prepares the request and sends it over a socket to the server, where the request is processed.

``` 
void populate_request(int fd, int access_code, char * action, char* filename, char * write_string, int read_bytes)
```
This function sends the request in the form of a **Request** struct:
```
typedef struct {
    int acces_code; //the access code of the user
    char action[50]; // To read or write
    char filename[128];  // file the user wnats to read or wort to
    char write_string[1024]; // IF write, the data the user wants to worite to the file
    int read_bytes; //If read, the number of bytes the user wants to read
}Request;
```

#### Access Control Logic:

The server will receive the request and verify whether the client has the required access level for the file they are trying to interact with. The access control logic ensures that the client can only perform actions (read/write) on files they are permitted to access based on their access level.

  ##### Access Level Check:
  - For each request, the server will check the client’s access_code.
  - The server compares the requested file (filename) with the files that the access level allows the client to access.
  - If the access code does not permit access to the requested file, the server will return an error.

  ##### Handling Actions:
  - If the action is read, the server will check if the requested file exists and then allow reading the specified number of bytes (read_bytes).
  - If the action is write, the server will check if the client is authorized to write to the specified file (filename). If authorized, the server will write the data from write_string to the file.

  ##### Error Handling:
  - If the access level is insufficient, the server will reject the request and return an error message.
  - If the action is not supported or the file is not found, the server will also return an error.

  #### Server Behavior:

The server will host the database files and listen for incoming client connections on a specified UNIX socket. Upon receiving a request, the server will:

  ##### Authenticate the Client: 
  - Once a client connects for the first time its sents its credential to the server to verfied it fails to do so it will be disconnected
  - Once the client credential has been verified it can now send requests to the server. The server then checks the provided **access_code** against the client's privileges and determine what files the client can access.
  ##### Process the Request:
  - If the request is a read action, the server will read the specified number of bytes from the requested file.
  - If the request is a write action, the server will write the provided data (write_string) to the requested file.
  - Return the Result: The server will send a success or error message back to the client, based on the outcome of the request.

  #### Example Workflow:

  - A client with access level 2 connects to the server and sends a request to read from **file1.txt**.
  - The server checks if the client's **access_code (2)** allows access to **file1.txt**. Since access level 2 includes **file1.txt**, the request is valid.
  - The client sends a request to read 100 bytes from **file1.txt**. The server reads the data and sends it back to the client.
  - If the client with access level 2 tries to access **file3.txt**, the server will reject the request with an error message because access level 2 does not allow access to **file3.txt**.

### Project Goals

Create a UNIX service that hosts a database and manages access control to different parts of the database based on the access level provided by the clients in their request.

The service should allow clients to interact with specific files in the database, with their access permissions governed by the access level they provide. Depending on their assigned access level, clients will have different read and write permissions for each file in the database.

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
  - Server can recieve incoming client connections through the name socket and save the client socket fd 
  table
  - Server can identify when any existing client already in the fd table is sending a request through POLLIN
- Database
  - Contributed to the development of the database concept for the project and assisted in its implementation
- Test
  - Tested the core functionality of the server, including handling multiple client connections and scenarios such as when a client fails to send credentials
- Documentation:
  - Overview and Project Goals
---
#### Treasure
- Event Loop
  - Server can recieve incoming client connections through the name socket and save the client socket fd 
  table
  - Server can identify when any existing client already in the fd table is sending a request through POLLIN
- Database
  - Contributed to the development of the database concept for the project and assisted in its implementation
### Testing
