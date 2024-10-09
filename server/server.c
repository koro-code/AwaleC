// server.c
#include "server.h"
#include <signal.h>


Room rooms[MAX_ROOMS];
int num_rooms;

// Structure pour stocker les joueurs déconnectés
typedef struct DisconnectedPlayer {
    char pseudo[MAX_PSEUDO_LENGTH];
    int room_id;
    int player_id;
    Room *room;
} DisconnectedPlayer;

DisconnectedPlayer disconnected_players[2];
int num_disconnected = 0;

void initialize_rooms(int num_rooms) {
    for (int i = 0; i < num_rooms; i++) {
        rooms[i].players_connected = 0;
        rooms[i].current_turn = 0;
        rooms[i].scores[0] = 0;
        rooms[i].scores[1] = 0;
        pthread_mutex_init(&rooms[i].room_mutex, NULL);
        // Initialiser le plateau avec 4 graines dans chaque case
        for (int row = 0; row < NUM_ROWS; row++) {
            for (int pit = 0; pit < NUM_PITS; pit++) {
                rooms[i].board[row][pit] = 4;
            }
        }
        // Initialiser les pointeurs de joueurs
        rooms[i].players[0] = NULL;
        rooms[i].players[1] = NULL;
    }
}

void update_score_file(const char *winner_pseudo) {
    FILE *file = fopen("scores.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s gagne une partie\n", winner_pseudo);
        fclose(file);
    }
}

int send_board_state(int socket, Room *room, int player_id) {
    char buffer[MAX_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "CLEAR_SCREEN\nBOARD_STATE\n%s", format_board(room, player_id));
    return send(socket, buffer, strlen(buffer), 0);
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
        // Avancer à la case suivante
        current_pit++;
        if (current_row == player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row; // Changer de ligne
            current_pit = 0;
        } else if (current_row != player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row; // Revenir à la ligne du joueur
            current_pit = 0;
        }

        // Ignorer la case de départ
        if (current_row == player_id && current_pit == pit_choice) {
            continue;
        }

        room->board[current_row][current_pit]++;
        seeds--;
    }

    // Logique simplifiée de capture
    // Vérifier si la dernière graine est tombée sur le côté adverse avec 2 ou 3 graines
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
    // Vérifier si un côté est vide
    int player1_empty = 1;
    int player2_empty = 1;
    for (int i = 0; i < NUM_PITS; i++) {
        if (room->board[0][i] > 0) player1_empty = 0;
        if (room->board[1][i] > 0) player2_empty = 0;
    }

    // Vérifier si un joueur a atteint 25 points ou plus
    int player1_score = room->scores[0];
    int player2_score = room->scores[1];
    
    if (player1_score >= 25 || player2_score >= 25) {
        return 1; // Fin de partie si un joueur a 25 points ou plus
    }

    return player1_empty || player2_empty;
}

int determine_winner(Room *room) {
    // Comparer les scores pour déterminer le gagnant
    if (room->scores[0] > room->scores[1]) return 0;
    else if (room->scores[1] > room->scores[0]) return 1;
    else return -1; // Match nul
}

void send_to_both_players(Room *room, const char *message) {
    for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
        if (room->players[i] != NULL) {
            send(room->players[i]->socket, message, strlen(message), 0);
        }
    }
}

void handle_disconnect(Player *player) {
    Room *room = &rooms[player->room_id];

    pthread_mutex_lock(&room->room_mutex);

    // Supprimer le joueur de la room
    room->players[player->player_id] = NULL;

    // Décrémenter le compteur de joueurs connectés
    room->players_connected--;

    printf("Après déconnexion de %s : players_connected = %d\n", player->pseudo, room->players_connected);

    // Enregistrer le joueur comme déconnecté
    strcpy(disconnected_players[num_disconnected].pseudo, player->pseudo);
    disconnected_players[num_disconnected].room_id = player->room_id;
    disconnected_players[num_disconnected].player_id = player->player_id;
    disconnected_players[num_disconnected].room = room;
    num_disconnected++;

    // Informer l'autre joueur
    int opponent_id = 1 - player->player_id;
    Player *opponent = room->players[opponent_id];
    if (opponent != NULL) {
        send(opponent->socket, "OPPONENT_DISCONNECTED", 21, 0);
    }

    pthread_mutex_unlock(&room->room_mutex);

    close(player->socket);
    free(player);
}




void *handle_client(void *arg) {
    Player *player = (Player *)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Recevoir le pseudo
    bytes_received = recv(player->socket, player->pseudo, MAX_PSEUDO_LENGTH, 0);
    if (bytes_received <= 0) {
        close(player->socket);
        free(player);
        pthread_exit(NULL);
    }
    player->pseudo[bytes_received] = '\0';

    // Vérifier si le joueur a une partie en cours
    int reconnected = 0;
    for (int i = 0; i < num_disconnected; i++) {
        if (strcmp(disconnected_players[i].pseudo, player->pseudo) == 0) {
            // Reconnexion du joueur
            player->room_id = disconnected_players[i].room_id;
            player->player_id = disconnected_players[i].player_id;
            Room *room = disconnected_players[i].room;
            room->players[player->player_id] = player;
            room->players_connected++; // Incrémenter le compteur de joueurs connectés

            // Supprimer le joueur de la liste des déconnectés
            for (int j = i; j < num_disconnected - 1; j++) {
                disconnected_players[j] = disconnected_players[j + 1];
            }
            num_disconnected--;

            send(player->socket, "RECONNECT", 9, 0);

            // Informer l'adversaire que le joueur s'est reconnecté
            int opponent_id = 1 - player->player_id;
            Player *opponent = room->players[opponent_id];
            if (opponent != NULL) {
                send(opponent->socket, "OPPONENT_RECONNECTED", 20, 0);
            }

            reconnected = 1;
            break;
        }
    }

    if (!reconnected) {
        while (1) {
            // Envoyer l'état des rooms
            snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
            for (int i = 0; i < num_rooms; i++) {
                char room_info[50];
                snprintf(room_info, sizeof(room_info), "Room %d: %d joueur(s) connecté(s)\n", i, rooms[i].players_connected);
                strcat(buffer, room_info);
            }
            send(player->socket, buffer, strlen(buffer), 0);

            // Recevoir la sélection de la room
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

            // Vérifier si le joueur est déjà dans la room (reconnexion)
            int already_in_room = 0;
            for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
                if (rooms[selected_room].players[i] != NULL && strcmp(rooms[selected_room].players[i]->pseudo, player->pseudo) == 0) {
                    already_in_room = 1;
                    break;
                }
            }

            if (rooms[selected_room].players_connected >= MAX_PLAYERS_PER_ROOM && !already_in_room) {
                pthread_mutex_unlock(&rooms[selected_room].room_mutex);
                send(player->socket, "ROOM_FULL", 9, 0);
                continue;
            }

            // Assigner le joueur à la room
            if (!already_in_room) {
                player->room_id = selected_room;
                player->player_id = rooms[selected_room].players_connected;
                rooms[selected_room].players[rooms[selected_room].players_connected++] = player;
            }

            pthread_mutex_unlock(&rooms[selected_room].room_mutex);

            send(player->socket, "JOINED_ROOM", 11, 0);
            break;
        }

        // Attendre que les deux joueurs soient connectés
        while (rooms[player->room_id].players_connected < MAX_PLAYERS_PER_ROOM) {
            sleep(1);
        }

        // Notifier les joueurs que la partie commence
        send(player->socket, "GAME_START", 10, 0);
    }

    // Boucle de jeu
    Room *room = &rooms[player->room_id];
    int opponent_id = 1 - player->player_id;
    Player *opponent;
    int game_over = 0;

    while (!game_over) {
    pthread_mutex_lock(&room->room_mutex);

    // Mettre à jour l'adversaire
    opponent = room->players[opponent_id];

    // Vérifier si l'adversaire est déconnecté
    if (opponent == NULL) {
        // Informer le joueur que l'adversaire est déconnecté
        send(player->socket, "OPPONENT_DISCONNECTED", 21, 0);
    }

    // Vérifier si c'est le tour du joueur
    if (room->current_turn == player->player_id) {
        // C'est le tour du joueur courant
        // Envoyer l'état du plateau au joueur
        send_board_state(player->socket, room, player->player_id);

        // Demander au joueur son coup
        send(player->socket, "YOUR_TURN\nEntrez le numéro de la case (1-6): ", 46, 0);

        // Recevoir le coup du joueur
        bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Gérer la déconnexion du joueur courant
            if (bytes_received == 0) {
                printf("Le joueur %s s'est déconnecté.\n", player->pseudo);
            } else {
                perror("Erreur lors de la réception des données");
            }

            // Déverrouiller le mutex avant d'appeler handle_disconnect
            pthread_mutex_unlock(&room->room_mutex);

            // Gérer la déconnexion
            handle_disconnect(player);

            pthread_exit(NULL);
        }
        buffer[bytes_received] = '\0';
        int pit_choice = atoi(buffer) - 1; // Convertir en index 0-based

        // Valider le coup
        if (pit_choice < 0 || pit_choice >= NUM_PITS || room->board[player->player_id][pit_choice] == 0) {
            send(player->socket, "INVALID_MOVE", 12, 0);
        } else {
            // Exécuter le coup
            execute_move(room, player->player_id, pit_choice);

            // Vérifier la condition de fin de partie
            if (is_game_over(room)) {
                game_over = 1;
                // Déterminer le gagnant et mettre à jour les scores
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
                // Changer de tour
                room->current_turn = opponent_id;

                // Envoyer le plateau mis à jour au joueur courant
                send_board_state(player->socket, room, player->player_id);

                // Vérifier si l'adversaire est connecté avant de lui envoyer le plateau
                if (opponent != NULL) {
                    // Envoyer le plateau mis à jour à l'adversaire
                    if (send_board_state(opponent->socket, room, opponent_id) <= 0) {
                        // L'envoi a échoué, l'adversaire est probablement déconnecté
                        printf("Le joueur %s s'est déconnecté.\n", opponent->pseudo);
                        // Supprimer l'adversaire de la room
                        handle_disconnect(opponent);
                    }
                } else {
                    // Informer le joueur courant que l'adversaire est déconnecté
                    send(player->socket, "OPPONENT_DISCONNECTED", 21, 0);
                }
            }
        }
    } else {
        // C'est le tour de l'adversaire
        if (opponent == NULL) {
            // L'adversaire est déconnecté, informer le joueur
            send(player->socket, "OPPONENT_DISCONNECTED", 21, 0);
        } else {
            // Envoyer un message d'attente
            if (send(player->socket, "WAITING_FOR_OPPONENT", 20, 0) <= 0) {
                // L'envoi a échoué, le joueur courant est déconnecté
                printf("Le joueur %s s'est déconnecté.\n", player->pseudo);
                pthread_mutex_unlock(&room->room_mutex);
                handle_disconnect(player);
                pthread_exit(NULL);
            }

            // Envoyer un message à l'adversaire pour détecter s'il est toujours connecté
            if (send(opponent->socket, "KEEP_ALIVE", 10, 0) <= 0) {
                // L'envoi a échoué, l'adversaire est déconnecté
                printf("Le joueur %s s'est déconnecté.\n", opponent->pseudo);
                handle_disconnect(opponent);
            }
        }
    }

    pthread_mutex_unlock(&room->room_mutex);
    sleep(1); // Pour éviter le busy waiting
}
  sleep(1); // Pour éviter le busy waiting
    
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    pthread_t thread_id;
    signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        printf("Usage: %s <nombre_de_rooms>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    num_rooms = atoi(argv[1]);
    if (num_rooms <= 0 || num_rooms > MAX_ROOMS) {
        printf("Nombre de rooms invalide. Le maximum autorisé est %d.\n", MAX_ROOMS);
        exit(EXIT_FAILURE);
    }

    initialize_rooms(num_rooms);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Échec de la création du socket");
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
        perror("Échec du bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("Échec du listen");
        exit(EXIT_FAILURE);
    }

    printf("Serveur démarré sur le port %d avec %d rooms.\n", PORT, num_rooms);

    while (1) {
        socklen_t sin_size = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &sin_size);
        if (client_socket < 0) {
            perror("Échec de l'accept");
            continue;
        }

        Player *new_player = (Player *)malloc(sizeof(Player));
        new_player->socket = client_socket;
        new_player->room_id = -1;

        if (pthread_create(&thread_id, NULL, handle_client, (void *)new_player) != 0) {
            perror("Échec de la création du thread");
            free(new_player);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_socket);
    return 0;
}
