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
#define MAX_PSEUDO_LENGTH 50
#define MAX_BUFFER_SIZE 2048
#define MAX_MESSAGE_LENGTH 256
#define CHAT_HISTORY_SIZE 100
#define PORT 12345

#define NUM_ROWS 2
#define NUM_PITS 6
// Définir le nombre maximum de joueurs connectés
#define MAX_CONNECTED_PLAYERS 100




typedef struct {
    int socket;
    char pseudo[MAX_PSEUDO_LENGTH];
    int room_id;
    int player_id;
    int in_chat_mode; // Indicateur de mode chat pour chaque joueur
    int has_sent_waiting_message; // Indicateur pour le message d'attente
} Player;


typedef struct {
    int players_connected;
    Player *players[MAX_PLAYERS_PER_ROOM];
    int board[NUM_ROWS][NUM_PITS];
    int scores[2];
    int current_turn;
    pthread_mutex_t room_mutex;
    // Ajout pour le chat
    char chat_history[CHAT_HISTORY_SIZE][MAX_MESSAGE_LENGTH];
    int chat_history_count;
} Room;

// Structure pour stocker les joueurs connectés
typedef struct {
    Player *players[MAX_CONNECTED_PLAYERS];
    int count;
} ConnectedPlayers;

extern ConnectedPlayers connected_players;


// Structure pour stocker les joueurs déconnectés
typedef struct DisconnectedPlayer {
    char pseudo[MAX_PSEUDO_LENGTH];
    int room_id;
    int player_id;
    Room *room;
} DisconnectedPlayer;

// Structure pour gérer les défis
typedef struct {
    char challenger[MAX_PSEUDO_LENGTH]; // Celui qui défie
    char challenged[MAX_PSEUDO_LENGTH]; // Celui qui est défié
    int is_active;                      // Si un défi est en cours ou non
} Challenge;

// Fonction pour envoyer un défi
void send_challenge(Player *challenger, Player *challenged);

// Fonction pour gérer la réponse à un défi
void handle_challenge_response(Player *challenged, int accepted);

// Fonctions
void *handle_client(void *arg);
void initialize_rooms(int num_rooms);
void update_score_file(const char *winner_pseudo);
int send_board_state(int socket, Room *room, int player_id);
char* format_board(Room *room, int player_id);
void execute_move(Room *room, int player_id, int pit_choice);
int is_game_over(Room *room);
int determine_winner(Room *room);
void send_to_both_players(Room *room, const char *message);
void handle_disconnect(Player *player);

#endif // SERVER_H
