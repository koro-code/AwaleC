#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_ROOMS 10
#define MAX_PLAYERS_PER_ROOM 2
#define MAX_PSEUDO_LENGTH 20
#define MAX_BUFFER_SIZE 1024
#define PORT 12345

typedef struct {
    int socket;
    char pseudo[MAX_PSEUDO_LENGTH];
    int room_id;
    int player_id;
} Player;

typedef struct {
    int players_connected;
    Player *players[MAX_PLAYERS_PER_ROOM];
    int board[2][6];
    int scores[2];
    int current_turn;
    pthread_mutex_t room_mutex;
} Room;

void *handle_client(void *arg);
void initialize_rooms(int num_rooms);
void update_score_file(const char *winner_pseudo);

#endif // SERVER_H
