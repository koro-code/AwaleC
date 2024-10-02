#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_PSEUDO_LENGTH 20
#define MAX_BUFFER_SIZE 1024
#define PORT 12345

void *receive_handler(void *socket_desc);

#endif // CLIENT_H
