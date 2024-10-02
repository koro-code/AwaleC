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
#define MAX_BUFFER_SIZE 2048
#define PORT 12345

#define NUM_ROWS 2
#define NUM_PITS 6

typedef struct {
    int socket;
    char pseudo[MAX_PSEUDO_LENGTH];
    int room_id;
    int player_id;
} Player;

typedef struct {
    int players_connected;
    Player *players[MAX_PLAYERS_PER_ROOM];
    int board[NUM_ROWS][NUM_PITS];
    int scores[2];
    int current_turn;
    pthread_mutex_t room_mutex;
} Room;

void *handle_client(void *arg);
void initialize_rooms(int num_rooms);
void update_score_file(const char *winner_pseudo);
void send_board_state(int socket, Room *room, int player_id);
char* format_board(Room *room, int player_id);
void execute_move(Room *room, int player_id, int pit_choice);
int is_game_over(Room *room);
int determine_winner(Room *room);
void send_to_both_players(Room *room, const char *message);

#endif // SERVER_H
