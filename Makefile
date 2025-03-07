CC = gcc
CFLAGS = -Wall -Wextra -Wno-int-conversion -std=c11
CLIENT_SRC = client.c
SERVER_SRC = server.c
SERVER_TEST_SRC = server_tests.c
TEST_FILES = test_1.c test_2.c test_3.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_TEST_OBJ = $(SERVER_TEST_SRC:.c=.o)
TEST_OBJS = $(TEST_FILES:.c=.o)

EXEC_CLIENT = client
EXEC_SERVER = server
EXEC_SERVER_TEST = server_tests
TEST_EXECUTABLES = $(TEST_FILES:.c=)

all: $(EXEC_CLIENT) $(EXEC_SERVER) $(EXEC_SERVER_TEST) $(TEST_EXECUTABLES)

$(EXEC_CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(EXEC_SERVER): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(EXEC_SERVER_TEST): $(SERVER_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build test executables
$(TEST_EXECUTABLES): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(SERVER_TEST_OBJ) $(TEST_OBJS) \
	      $(EXEC_CLIENT) $(EXEC_SERVER) $(EXEC_SERVER_TEST) $(TEST_EXECUTABLES) \
	      tests/output/*.out

.PHONY: all clean test
