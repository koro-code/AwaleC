# Makefile for compiling the Awalé game

CC = gcc
CFLAGS = -Wall -pthread

SERVER_BIN = server/awale_server
CLIENT_BIN = client/awale_client

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): server/server.c server/server.h
	$(CC) $(CFLAGS) -o $(SERVER_BIN) server/server.c

$(CLIENT_BIN): client/client.c client/client.h
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) client/client.c

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
