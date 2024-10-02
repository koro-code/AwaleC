#include "server.h"

Room rooms[MAX_ROOMS];
int num_rooms;

void initialize_rooms(int num_rooms) {
    for (int i = 0; i < num_rooms; i++) {
        rooms[i].players_connected = 0;
        rooms[i].current_turn = 0;
        rooms[i].scores[0] = 0;
        rooms[i].scores[1] = 0;
        pthread_mutex_init(&rooms[i].room_mutex, NULL);
        // Initialize the board with 4 seeds in each pit
        for (int row = 0; row < NUM_ROWS; row++) {
            for (int pit = 0; pit < NUM_PITS; pit++) {
                rooms[i].board[row][pit] = 4;
            }
        }
    }
}

void update_score_file(const char *winner_pseudo) {
    FILE *file = fopen("scores.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s wins a game\n", winner_pseudo);
        fclose(file);
    }
}

void send_board_state(int socket, Room *room, int player_id) {
    char buffer[MAX_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "BOARD_STATE\n%s", format_board(room, player_id));
    send(socket, buffer, strlen(buffer), 0);
}

char* format_board(Room *room, int player_id) {
    static char board_str[2048];
    int opponent_id = 1 - player_id;

    snprintf(board_str, sizeof(board_str), "\n\t\t     Joueur %d\n", opponent_id + 1);
    strcat(board_str, "\t[1]   [2]   [3]   [4]   [5]   [6]\n");
    strcat(board_str, " +----+-----+-----+-----+-----+-----+-----+----+\n");
    snprintf(board_str + strlen(board_str), sizeof(board_str) - strlen(board_str),
             " |    |  %2d |  %2d |  %2d |  %2d |  %2d |  %2d |    |\n",
             room->board[opponent_id][5], room->board[opponent_id][4], room->board[opponent_id][3],
             room->board[opponent_id][2], room->board[opponent_id][1], room->board[opponent_id][0]);
    snprintf(board_str + strlen(board_str), sizeof(board_str) - strlen(board_str),
             " | %2d +-----+-----+-----+-----+-----+-----+ %2d |\n",
             room->scores[player_id], room->scores[opponent_id]);
    snprintf(board_str + strlen(board_str), sizeof(board_str) - strlen(board_str),
             " |    |  %2d |  %2d |  %2d |  %2d |  %2d |  %2d |    |\n",
             room->board[player_id][0], room->board[player_id][1], room->board[player_id][2],
             room->board[player_id][3], room->board[player_id][4], room->board[player_id][5]);
    strcat(board_str, " +----+-----+-----+-----+-----+-----+-----+----+\n");
    strcat(board_str, "\t[1]   [2]   [3]   [4]   [5]   [6]\n");
    snprintf(board_str + strlen(board_str), sizeof(board_str) - strlen(board_str),
             "\t\t     Joueur %d\n", player_id + 1);

    return board_str;
}

void execute_move(Room *room, int player_id, int pit_choice) {
    int seeds = room->board[player_id][pit_choice];
    room->board[player_id][pit_choice] = 0;
    int current_row = player_id;
    int current_pit = pit_choice;

    while (seeds > 0) {
        // Move to next pit
        current_pit++;
        if (current_row == player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row; // Switch row
            current_pit = 0;
        } else if (current_row != player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row; // Switch back to player's row
            current_pit = 0;
        }

        // Skip the starting pit
        if (current_row == player_id && current_pit == pit_choice) {
            continue;
        }

        room->board[current_row][current_pit]++;
        seeds--;
    }

    // Simplified capture logic
    // Check if the last seed landed on the opponent's side with 2 or 3 seeds
    if (current_row != player_id) {
        int captured_seeds = 0;
        while (current_pit >= 0 && (room->board[current_row][current_pit] == 2 || room->board[current_row][current_pit] == 3)) {
            captured_seeds += room->board[current_row][current_pit];
            room->board[current_row][current_pit] = 0;
            current_pit--;
        }
        room->scores[player_id] += captured_seeds;
    }
}

int is_game_over(Room *room) {
    // Check if one side is empty
    int player1_empty = 1;
    int player2_empty = 1;
    for (int i = 0; i < NUM_PITS; i++) {
        if (room->board[0][i] > 0) player1_empty = 0;
        if (room->board[1][i] > 0) player2_empty = 0;
    }

    // Check if any player has reached 25 or more points
    int player1_score = room->scores[0];
    int player2_score = room->scores[1];
    
    if (player1_score >= 25 || player2_score >= 25) {
        return 1; // Game over if a player has 25 or more points
    }

    return player1_empty || player2_empty;
}


int determine_winner(Room *room) {
    // Compare scores to determine the winner
    if (room->scores[0] > room->scores[1]) return 0;
    else if (room->scores[1] > room->scores[0]) return 1;
    else return -1; // Draw
}

void send_to_both_players(Room *room, const char *message) {
    for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
        send(room->players[i]->socket, message, strlen(message), 0);
    }
}

void *handle_client(void *arg) {
    Player *player = (Player *)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Receive pseudo
    bytes_received = recv(player->socket, player->pseudo, MAX_PSEUDO_LENGTH, 0);
    if (bytes_received <= 0) {
        close(player->socket);
        free(player);
        pthread_exit(NULL);
    }
    player->pseudo[bytes_received] = '\0';

    while (1) {
        // Send room status
        snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
        for (int i = 0; i < num_rooms; i++) {
            char room_info[50];
            snprintf(room_info, sizeof(room_info), "Room %d: %d player(s) connected\n", i, rooms[i].players_connected);
            strcat(buffer, room_info);
        }
        send(player->socket, buffer, strlen(buffer), 0);

        // Receive room selection
        bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            close(player->socket);
            free(player);
            pthread_exit(NULL);
        }
        buffer[bytes_received] = '\0';
        int selected_room = atoi(buffer);
        if (selected_room < 0 || selected_room >= num_rooms) {
            send(player->socket, "INVALID_ROOM", 12, 0);
            continue;
        }

        pthread_mutex_lock(&rooms[selected_room].room_mutex);
        if (rooms[selected_room].players_connected >= MAX_PLAYERS_PER_ROOM) {
            pthread_mutex_unlock(&rooms[selected_room].room_mutex);
            send(player->socket, "ROOM_FULL", 9, 0);
            continue;
        }

        // Assign player to room
        player->room_id = selected_room;
        player->player_id = rooms[selected_room].players_connected;
        rooms[selected_room].players[rooms[selected_room].players_connected++] = player;
        pthread_mutex_unlock(&rooms[selected_room].room_mutex);

        send(player->socket, "JOINED_ROOM", 11, 0);
        break;
    }

    // Wait for both players to connect
    while (rooms[player->room_id].players_connected < MAX_PLAYERS_PER_ROOM) {
        sleep(1);
    }

    // Notify players that the game is starting
    send(player->socket, "GAME_START", 10, 0);

    // Game loop
    Room *room = &rooms[player->room_id];
    int opponent_id = 1 - player->player_id;
    Player *opponent = room->players[opponent_id];
    int game_over = 0;

    while (!game_over) {
        pthread_mutex_lock(&room->room_mutex);

        // Check if it's this player's turn
        if (room->current_turn == player->player_id) {
            // Send the board state to the player
            send_board_state(player->socket, room, player->player_id);

            // Prompt the player for their move
            send(player->socket, "YOUR_TURN\nEnter the pit number (1-6): ", 39, 0);

            // Receive the player's move
            bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                // Handle disconnection
                send(opponent->socket, "OPPONENT_DISCONNECTED", 21, 0);
                update_score_file(opponent->pseudo);
                close(player->socket);
                free(player);
                pthread_mutex_unlock(&room->room_mutex);
                pthread_exit(NULL);
            }
            buffer[bytes_received] = '\0';
            int pit_choice = atoi(buffer) - 1; // Convert to 0-based index

            // Validate the move
            if (pit_choice < 0 || pit_choice >= NUM_PITS || room->board[player->player_id][pit_choice] == 0) {
                send(player->socket, "INVALID_MOVE", 12, 0);
            } else {
                // Execute the move
                execute_move(room, player->player_id, pit_choice);

                // Check for game over condition
                if (is_game_over(room)) {
                    game_over = 1;
                    // Determine winner and update scores
                    int winner_id = determine_winner(room);
                    if (winner_id == -1) {
                        send_to_both_players(room, "GAME_OVER\nMatch nul !");
                    } else {
                        char win_msg[50];
                        snprintf(win_msg, sizeof(win_msg), "GAME_OVER\n%s gagne la partie !", room->players[winner_id]->pseudo);
                        send_to_both_players(room, win_msg);
                        update_score_file(room->players[winner_id]->pseudo);
                    }
                } else {
                    // Switch turns
                    room->current_turn = opponent_id;
                    // Send updated board to both players
                    send_board_state(opponent->socket, room, opponent_id);
                    send_board_state(player->socket, room, player->player_id);
                }
            }
        } else {
            // Send waiting message
            send(player->socket, "WAITING_FOR_OPPONENT", 20, 0);
        }

        pthread_mutex_unlock(&room->room_mutex);
        sleep(1); // Prevent busy waiting
    }

    // Close connections
    close(player->socket);
    free(player);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;

    if (argc != 2) {
        printf("Usage: %s <number_of_rooms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    num_rooms = atoi(argv[1]);
    if (num_rooms <= 0 || num_rooms > MAX_ROOMS) {
        printf("Invalid number of rooms. Max allowed is %d.\n", MAX_ROOMS);
        exit(EXIT_FAILURE);
    }

    initialize_rooms(num_rooms);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d with %d rooms.\n", PORT, num_rooms);

    while (1) {
        socklen_t sin_size = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &sin_size);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        Player *new_player = (Player *)malloc(sizeof(Player));
        new_player->socket = client_socket;
        new_player->room_id = -1;

        if (pthread_create(&thread_id, NULL, handle_client, (void *)new_player) != 0) {
            perror("Thread creation failed");
            free(new_player);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}
