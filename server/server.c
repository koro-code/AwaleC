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
        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 6; k++)
                rooms[i].board[j][k] = 4;
    }
}

void update_score_file(const char *winner_pseudo) {
    FILE *file = fopen("scores.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s wins a game\n", winner_pseudo);
        fclose(file);
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

    // Send initial room status
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
    while (1) {
        // Lock the room during the game turn
        pthread_mutex_lock(&room->room_mutex);

        // Check if it's this player's turn
        if (room->current_turn == player->player_id) {
            // Send the board state to the player
            // For simplicity, we'll send a placeholder board state
            snprintf(buffer, sizeof(buffer), "YOUR_TURN\nBoard State: ...\nEnter your move:");
            send(player->socket, buffer, strlen(buffer), 0);

            // Receive the player's move
            bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                // Handle disconnection
                int opponent_id = 1 - player->player_id;
                Player *opponent = room->players[opponent_id];
                send(opponent->socket, "OPPONENT_DISCONNECTED", 21, 0);
                update_score_file(opponent->pseudo);
                close(player->socket);
                free(player);
                pthread_mutex_unlock(&room->room_mutex);
                pthread_exit(NULL);
            }
            buffer[bytes_received] = '\0';

            // Process the move (this is where game logic would go)
            // For now, we'll just switch turns
            room->current_turn = 1 - room->current_turn;

            // Send updated board state to both players
            snprintf(buffer, sizeof(buffer), "BOARD_UPDATE\nBoard State: ...\n");
            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
                send(room->players[i]->socket, buffer, strlen(buffer), 0);
            }
        } else {
            // Send waiting message
            snprintf(buffer, sizeof(buffer), "WAITING_FOR_OPPONENT\n");
            send(player->socket, buffer, strlen(buffer), 0);
        }

        pthread_mutex_unlock(&room->room_mutex);

        // Small delay to prevent busy waiting
        sleep(1);
    }

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
