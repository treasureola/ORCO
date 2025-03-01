CC = gcc
CFLAGS = -Wall -Wextra -std=c11
CLIENT_SRC = client.c
SERVER_SRC = server.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
EXEC_CLIENT = client
EXEC_SERVER = server

all: $(EXEC_CLIENT) $(EXEC_SERVER)

$(EXEC_CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(EXEC_SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(EXEC_CLIENT) $(EXEC_SERVER)

.PHONY: all clean
