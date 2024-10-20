// server.c

#include "server.h"

PlayerAccount player_accounts[MAX_PLAYERS];
int num_accounts = 0;

Room rooms[MAX_ROOMS];
int num_rooms;

DisconnectedPlayer disconnected_players[2];
int num_disconnected = 0;
ConnectedPlayers connected_players = { .count = 0 };


Challenge challenge;

void initialize_challenge() {
    challenge.is_active = 0;
}

void send_connected_players_list(int client_socket) {
    char buffer[MAX_BUFFER_SIZE] = "Liste des joueurs connectés :\n";
    for (int i = 0; i < connected_players.count; i++) {
        if (connected_players.players[i] != NULL) {
            strcat(buffer, connected_players.players[i]->pseudo);
            strcat(buffer, "\n");
        }
    }
    send(client_socket, buffer, strlen(buffer), 0);
}

int find_player_by_pseudo(const char *pseudo) {
    for (int i = 0; i < num_accounts; i++) {
        if (strcmp(player_accounts[i].pseudo, pseudo) == 0) {
            return i; // Renvoie l'indice du joueur
        }
    }
    return -1; // Le joueur n'existe pas
}

int create_new_player(const char *pseudo, const char *password, const char *bio) {
    if (num_accounts >= MAX_PLAYERS) {
        return -1; // Limite des comptes atteinte
    }
    strncpy(player_accounts[num_accounts].pseudo, pseudo, MAX_PSEUDO_LENGTH);
    strncpy(player_accounts[num_accounts].password, password, MAX_PASSWORD_LENGTH);
    strncpy(player_accounts[num_accounts].bio, bio, MAX_BIO_LENGTH);
    num_accounts++;
    return 0; // Compte créé avec succès
}



// Fonction pour envoyer un défi à un joueur
void send_challenge(Player *challenger, Player *challenged) {
    if (challenge.is_active) {
        send(challenger->socket, "Un défi est déjà en cours.\n", 27, 0);
        return;
    }

    strncpy(challenge.challenger, challenger->pseudo, MAX_PSEUDO_LENGTH);
    strncpy(challenge.challenged, challenged->pseudo, MAX_PSEUDO_LENGTH);
    challenge.is_active = 1;

    char buffer[MAX_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Vous avez été défié par %s. Tapez /accepter pour accepter ou /refuser pour refuser.\n", challenger->pseudo);
    send(challenged->socket, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "Défi envoyé à %s. En attente de sa réponse.\n", challenged->pseudo);
    send(challenger->socket, buffer, strlen(buffer), 0);
}

void add_connected_player(Player *player) {
    if (connected_players.count < MAX_CONNECTED_PLAYERS) {
        connected_players.players[connected_players.count++] = player;
    } else {
        printf("Limite des joueurs connectés atteinte.\n");
    }
}

void remove_connected_player(Player *player) {
    for (int i = 0; i < connected_players.count; i++) {
        if (connected_players.players[i] == player) {
            for (int j = i; j < connected_players.count - 1; j++) {
                connected_players.players[j] = connected_players.players[j + 1];
            }
            connected_players.players[--connected_players.count] = NULL;
            break;
        }
    }
}



void handle_challenge_response(Player *challenger, Player *challenged, int accepted) {
    if (!challenge.is_active || strcmp(challenge.challenged, challenged->pseudo) != 0) {
        send(challenged->socket, "Aucun défi en cours pour vous.\n", 30, 0);
        return;
    }

    char buffer[MAX_BUFFER_SIZE];

    if (accepted) {
        Player *opponent;
        // Si le défi est accepté

        if (challenger == NULL) {

            send(challenged->socket, "Vous avez accepté le défi !\n", 29, 0);

            challenged->room_id = 0;
            challenged->player_id = rooms[0].players_connected;
            rooms[0].players[rooms[0].players_connected++] = challenged;

            game_loop(opponent, challenged, &rooms[0]);

        } else if (challenged == NULL) {

            // send(challenger->socket, "Le defi va etre lancé\n", 23, 0);

            // challenger->room_id = 0;
            // challenger->player_id = rooms[0].players_connected;
            // rooms[0].players[rooms[0].players_connected++] = challenger;



        }



        // snprintf(buffer, sizeof(buffer), "%s a accepté votre défi !\n", challenged->pseudo);
        // send(challenger->socket, buffer, strlen(buffer), 0);

        // send(challenged->socket, "Vous avez accepté le défi !\n", 29, 0);

        // // if (!already_in_room) {
        // //     player->room_id = selected_room;
        // //     player->player_id = rooms[selected_room].players_connected;
        // //     rooms[selected_room].players[rooms[selected_room].players_connected++] = player;
        // // }
        // // Assigner les deux joueurs à la salle 0
  
        // challenged->room_id = 0;
        // challenged->player_id = rooms[0].players_connected;
        // rooms[0].players[rooms[0].players_connected++] = challenged;

        // challenger->room_id = 0;
        // challenger->player_id = rooms[0].players_connected;
        // rooms[0].players[rooms[0].players_connected++] = challenger;



        // // Lancer la boucle de jeu en appelant la fonction game_loop avec les bons arguments
        // game_loop(challenger, challenged, &rooms[0]);
    
    } else {
        // Si le défi est refusé
        snprintf(buffer, sizeof(buffer), "%s a refusé votre défi.\n", challenged->pseudo);
        send(challenger->socket, buffer, strlen(buffer), 0);
        send(challenged->socket, "Vous avez refusé le défi.\n", 27, 0);
    }

    // Réinitialiser l'état du défi après une réponse
    challenge.is_active = 0;
}




void initialize_rooms(int num_rooms) {
    for (int i = 0; i < num_rooms; i++) {
        rooms[i].players_connected = 0;
        rooms[i].current_turn = 0;
        rooms[i].scores[0] = 0;
        rooms[i].scores[1] = 0;
        pthread_mutex_init(&rooms[i].room_mutex, NULL);
        rooms[i].chat_history_count = 0;

        for (int row = 0; row < NUM_ROWS; row++) {
            for (int pit = 0; pit < NUM_PITS; pit++) {
                rooms[i].board[row][pit] = 4;
            }
        }
        rooms[i].players[0] = NULL;
        rooms[i].players[1] = NULL;
    }
}

void reset_room(Room *room) {
    for (int row = 0; row < NUM_ROWS; row++) {
        for (int pit = 0; pit < NUM_PITS; pit++) {
            room->board[row][pit] = 4;
        }
    }
    room->scores[0] = 0;
    room->scores[1] = 0;
    room->current_turn = 0;
    room->chat_history_count = 0;
    room->players_connected = 0;
    room->players[0] = NULL;
    room->players[1] = NULL;
}

void update_score_file(const char *winner_pseudo) {
    FILE *file = fopen("scores.txt", "r");
    if (!file) {
        file = fopen("scores.txt", "w");
        if (file) {
            fprintf(file, "%s %d\n", winner_pseudo, 1);
            fclose(file);
        }
        return;
    }

    char line[MAX_BUFFER_SIZE];
    int found = 0;

    // Stocker les scores en mémoire
    typedef struct {
        char pseudo[MAX_PSEUDO_LENGTH];
        int score;
    } ScoreEntry;

    ScoreEntry scores[100];
    int num_scores = 0;

    while (fgets(line, sizeof(line), file)) {
        char pseudo[MAX_PSEUDO_LENGTH];
        int score;
        if (sscanf(line, "%s %d", pseudo, &score) == 2) {
            if (strcmp(pseudo, winner_pseudo) == 0) {
                score += 1;
                found = 1;
            }
            strcpy(scores[num_scores].pseudo, pseudo);
            scores[num_scores].score = score;
            num_scores++;
        }
    }
    fclose(file);

    if (!found) {
        // Le gagnant n'est pas dans la liste, l'ajouter
        strcpy(scores[num_scores].pseudo, winner_pseudo);
        scores[num_scores].score = 1;
        num_scores++;
    }

    // Écrire les scores mis à jour dans le fichier
    file = fopen("scores.txt", "w");
    if (file) {
        for (int i = 0; i < num_scores; i++) {
            fprintf(file, "%s %d\n", scores[i].pseudo, scores[i].score);
        }
        fclose(file);
    }
}

void send_scoreboard(int client_socket) {
    FILE *file = fopen("scores.txt", "r");
    if (!file) {
        send(client_socket, "SCOREBOARD\nAucun score disponible.\n", 35, 0);
        return;
    }

    char buffer[MAX_BUFFER_SIZE];
    strcpy(buffer, "SCOREBOARD\n");

    char line[MAX_BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        strcat(buffer, line);
    }
    fclose(file);

    send(client_socket, buffer, strlen(buffer), 0);
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
        current_pit++;
        if (current_row == player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row;
            current_pit = 0;
        } else if (current_row != player_id && current_pit == NUM_PITS) {
            current_row = 1 - current_row;
            current_pit = 0;
        }

        // Ignorer la case de départ
        if (current_row == player_id && current_pit == pit_choice) {
            continue;
        }

        room->board[current_row][current_pit]++;
        seeds--;
    }

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
    int player1_empty = 1;
    int player2_empty = 1;
    for (int i = 0; i < NUM_PITS; i++) {
        if (room->board[0][i] > 0) player1_empty = 0;
        if (room->board[1][i] > 0) player2_empty = 0;
    }

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
    else return -1;
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

    room->players[player->player_id] = NULL;

    room->players_connected--;

    printf("Après déconnexion de %s : players_connected = %d\n", player->pseudo, room->players_connected);

    // Enregistrer le joueur comme déconnecté
    strcpy(disconnected_players[num_disconnected].pseudo, player->pseudo);
    disconnected_players[num_disconnected].room_id = player->room_id;
    disconnected_players[num_disconnected].player_id = player->player_id;
    disconnected_players[num_disconnected].room = room;
    num_disconnected++;

    int opponent_id = 1 - player->player_id;
    Player *opponent = room->players[opponent_id];
    if (opponent != NULL) {
        send(opponent->socket, "OPPONENT_DISCONNECTED", 21, 0);
    }

    pthread_mutex_unlock(&room->room_mutex);

    close(player->socket);
    free(player);
}

void send_chat_history(Room *room, Player *player) {
    char chat_history_buffer[MAX_BUFFER_SIZE];
    strcpy(chat_history_buffer, "CHAT_HISTORY\n");
    
    // Ajouter chaque message de l'historique du chat
    for (int i = 0; i < room->chat_history_count; i++) {
        strcat(chat_history_buffer, room->chat_history[i]);
        strcat(chat_history_buffer, "\n");
    }
    
    // Envoyer l'historique du chat au joueur
    send(player->socket, chat_history_buffer, strlen(chat_history_buffer), 0);
}


void handle_player_move(Room *room, Player *player, char *buffer) {
    int pit_choice = atoi(buffer) - 1;

    if (pit_choice < 0 || pit_choice >= NUM_PITS || room->board[player->player_id][pit_choice] == 0) {
        // Mouvement invalide
        send(player->socket, "INVALID_MOVE", 12, 0);
    } else {
        // Exécuter le mouvement et mettre à jour le plateau
        execute_move(room, player->player_id, pit_choice);

        // Envoyer l'état du plateau après le mouvement
        send_board_state(player->socket, room, player->player_id);
    }
}

void handle_game_over(Room *room, Player *player, Player *opponent) {
    int winner_id = determine_winner(room);

    if (winner_id == -1) {
        // Match nul
        send_to_both_players(room, "GAME_OVER\nMatch nul !");
    } else {
        // Gagnant trouvé
        char win_msg[50];
        snprintf(win_msg, sizeof(win_msg), "GAME_OVER\n%s gagne la partie !", room->players[winner_id]->pseudo);
        send_to_both_players(room, win_msg);
        update_score_file(room->players[winner_id]->pseudo);
    }

    // Réinitialiser la salle
    room->players[player->player_id] = NULL;
    room->players[1 - player->player_id] = NULL;
    room->players_connected = 0;
    reset_room(room);
}

void update_turn(Room *room, Player *opponent) {
    room->current_turn = 1 - room->current_turn;  // Changer de joueur

    // Réinitialiser les messages d'attente
    for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
        if (room->players[i] != NULL) {
            room->players[i]->has_sent_waiting_message = 0;
        }
    }

    // Envoyer l'état du plateau à l'adversaire
    if (opponent != NULL) {
        send_board_state(opponent->socket, room, 1 - room->current_turn);
    }
}


void handle_waiting_opponent(Player *player, Player *opponent) {
    if (opponent == NULL) {
        // L'adversaire est déconnecté
        send(player->socket, "OPPONENT_DISCONNECTED", 21, 0);
    } else {
        // Envoyer un message d'attente
        if (player->has_sent_waiting_message == 0) {
            send(player->socket, "WAITING_FOR_OPPONENT", 20, 0);
            player->has_sent_waiting_message = 1;
        }

        // Garder l'adversaire actif
        if (send(opponent->socket, "KEEP_ALIVE", 10, 0) <= 0) {
            printf("Le joueur %s s'est déconnecté.\n", opponent->pseudo);
            handle_disconnect(opponent);
        }
    }
}

void ask_for_rematch(Player *player) {
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    // Demander au joueur s'il veut rejouer
    send(player->socket, "GAME_OVER\nVoulez-vous rejouer ? (oui/non): ", 45, 0);

    bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        handle_disconnect_on_error(player, bytes_received);
    }

    buffer[bytes_received] = '\0';

    if (strcasecmp(buffer, "oui") == 0) {
        // Réinitialiser les données du joueur
        player->room_id = -1;
        player->player_id = -1;
        player->in_chat_mode = 0;
        player->has_sent_waiting_message = 0;
    } else {
        // Terminer la connexion du joueur
        close(player->socket);
        free(player);
        pthread_exit(NULL);
    }
}


void handle_disconnect_on_error(Player *player, int bytes_received) {
    if (bytes_received == 0) {
        printf("Le joueur %s s'est déconnecté.\n", player->pseudo);
    } else {
        perror("Erreur lors de la réception des données");
    }

    handle_disconnect(player);
    pthread_exit(NULL);
}


void game_loop(Player *player, Player *opponent, Room *room) {
    int opponent_id = 1 - player->player_id;
    int bytes_received;
    char buffer[MAX_BUFFER_SIZE];
    int game_over = 0;

    while (!game_over) {
        pthread_mutex_lock(&room->room_mutex);

        if (opponent == NULL)
            opponent = room->players[opponent_id];

        if (opponent == NULL) {
            send(player->socket, "OPPONENT_DISCONNECTED", 21, 0);
        }

        pthread_mutex_unlock(&room->room_mutex); 
        bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, MSG_DONTWAIT);
        pthread_mutex_lock(&room->room_mutex); 

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';

            if (strcmp(buffer, "/chat") == 0) {
                player->in_chat_mode = 1;
                send(player->socket, "ENTER_CHAT_MODE", 15, 0);

                char chat_history_buffer[MAX_BUFFER_SIZE];
                strcpy(chat_history_buffer, "CHAT_HISTORY\n");
                for (int i = 0; i < room->chat_history_count; i++) {
                    strcat(chat_history_buffer, room->chat_history[i]);
                    strcat(chat_history_buffer, "\n");
                }
                send(player->socket, chat_history_buffer, strlen(chat_history_buffer), 0);

                pthread_mutex_unlock(&room->room_mutex);
                continue;
            } else if (strcmp(buffer, "/game") == 0) {
                player->in_chat_mode = 0;
                send(player->socket, "ENTER_GAME_MODE", 15, 0);

                if (room->current_turn != player->player_id) {
                    player->has_sent_waiting_message = 0;
                }

                send_board_state(player->socket, room, player->player_id);

                pthread_mutex_unlock(&room->room_mutex);
                continue;
            } else if (strcmp(buffer, "/score") == 0) {
                send_scoreboard(player->socket);
                sleep(5);
                pthread_mutex_unlock(&room->room_mutex);
                continue;
            } else if (strncmp(buffer, "/bio", 4) == 0) {
                // Demander le pseudo du joueur pour lequel on veut voir la biographie
                send(player->socket, "Entrez le pseudonyme du joueur : ", 33, 0);
                
                // Recevoir le pseudo du joueur demandé
                bytes_received = recv(player->socket, buffer, MAX_PSEUDO_LENGTH, 0);
                if (bytes_received <= 0) {
                    close(player->socket);
                    free(player);
                    pthread_exit(NULL);
                }
                buffer[bytes_received] = '\0';
                // Rechercher le joueur par pseudo
                int player_index = find_player_by_pseudo(buffer);
                if (player_index >= 0) {
                    // Envoyer la biographie du joueur
                    char bio_message[MAX_BUFFER_SIZE];
                    snprintf(bio_message, sizeof(bio_message), "Biographie de %s : %s\n", player_accounts[player_index].pseudo, player_accounts[player_index].bio);
                    send(player->socket, bio_message, strlen(bio_message), 0);
                } else {
                    // Si le joueur n'existe pas
                    send(player->socket, "Joueur introuvable ou sans biographie.\n", 39, 0);
                }
            } else if (strcmp(buffer, "/liste") == 0) {
                send_connected_players_list(player->socket);

                // Attendre 5 secondes avant de réafficher le lobby
                sleep(5);

                // Réafficher l'état du lobby après le délai
                snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
                for (int i = 0; i < num_rooms; i++) {
                    char room_info[50];
                    snprintf(room_info, sizeof(room_info), "Room %d: %d joueur(s) connecté(s)\n", i, rooms[i].players_connected);
                    strcat(buffer, room_info);
                }
                send(player->socket, buffer, strlen(buffer), 0);
            }



            if (player->in_chat_mode) {
            
                if (room->chat_history_count < CHAT_HISTORY_SIZE) {
                    snprintf(room->chat_history[room->chat_history_count], MAX_MESSAGE_LENGTH, "%s: %s", player->pseudo, buffer);
                    room->chat_history_count++;
                } else {
                    for (int i = 1; i < CHAT_HISTORY_SIZE; i++) {
                        strcpy(room->chat_history[i - 1], room->chat_history[i]);
                    }
                    snprintf(room->chat_history[CHAT_HISTORY_SIZE - 1], MAX_MESSAGE_LENGTH, "%s: %s", player->pseudo, buffer);
                }
                for (int i = 0; i < MAX_PLAYERS_PER_ROOM; i++) {
                    if (room->players[i] != NULL) {
                        char chat_update[MAX_BUFFER_SIZE];
                        snprintf(chat_update, sizeof(chat_update), "CHAT_UPDATE\n%s: %s", player->pseudo, buffer);
                        send(room->players[i]->socket, chat_update, strlen(chat_update), 0);
                    }
                }

                pthread_mutex_unlock(&room->room_mutex);
                continue;
            }
            // Gestion des autres commandes
            // (Similar logic for /bio, /liste, chat mode handling, and game move validation)
        } else if (bytes_received == 0) {
            printf("Le joueur %s s'est déconnecté.\n", player->pseudo);
            pthread_mutex_unlock(&room->room_mutex);
            handle_disconnect(player);
            pthread_exit(NULL);
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Erreur lors de la réception des données");
                pthread_mutex_unlock(&room->room_mutex);
                handle_disconnect(player);
                pthread_exit(NULL);
            }
        }

        // Tour du joueur
        if (room->current_turn == player->player_id && !player->in_chat_mode) {
            send_board_state(player->socket, room, player->player_id);
            send(player->socket, "YOUR_TURN\nEntrez le numéro de la case (1-6): ", 46, 0);

            bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                handle_disconnect_on_error(player, bytes_received);
            }

            buffer[bytes_received] = '\0';

            if (strcmp(buffer, "/chat") == 0) {
                    player->in_chat_mode = 1;
                    send(player->socket, "ENTER_CHAT_MODE", 15, 0);

                    // Envoyer l'historique du chat au joueur
                    char chat_history_buffer[MAX_BUFFER_SIZE];
                    strcpy(chat_history_buffer, "CHAT_HISTORY\n");
                    for (int i = 0; i < room->chat_history_count; i++) {
                        strcat(chat_history_buffer, room->chat_history[i]);
                        strcat(chat_history_buffer, "\n");
                    }
                    send(player->socket, chat_history_buffer, strlen(chat_history_buffer), 0);

                    pthread_mutex_unlock(&room->room_mutex);
                    continue;
                } else if (strcmp(buffer, "/score") == 0) {
                    // Envoyer le tableau des scores au joueur
                    send_scoreboard(player->socket);

                    sleep(5);

                pthread_mutex_unlock(&room->room_mutex);
                continue;
            } else if (strcmp(buffer, "/liste") == 0) {
                send_connected_players_list(player->socket);

                // Attendre 5 secondes avant de réafficher le lobby
                sleep(5);

                // Réafficher l'état du lobby après le délai
                snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
                for (int i = 0; i < num_rooms; i++) {
                    char room_info[50];
                    snprintf(room_info, sizeof(room_info), "Room %d: %d joueur(s) connecté(s)\n", i, rooms[i].players_connected);
                    strcat(buffer, room_info);
                }
                send(player->socket, buffer, strlen(buffer), 0);
            }


            // Gestion du tour et de la capture
            handle_player_move(room, player, buffer);

            if (is_game_over(room)) {
                game_over = 1;
                handle_game_over(room, player, opponent);
            } else {
                update_turn(room, opponent);
            }
        } else if (!player->in_chat_mode) {
            handle_waiting_opponent(player, opponent);
        }

        pthread_mutex_unlock(&room->room_mutex);
        sleep(1);
    }

    ask_for_rematch(player);
}


void *handle_client(void *arg) {
    Player *player = (Player *)arg;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;
    player->has_sent_waiting_message = 0;
    int challenge_accepted = 0;


    bytes_received = recv(player->socket, player->pseudo, MAX_PSEUDO_LENGTH, 0);
    if (bytes_received <= 0) {
        close(player->socket);
        free(player);
        pthread_exit(NULL);
    }
    player->pseudo[bytes_received] = '\0';
    player->in_chat_mode = 0;

    // Vérifier si le pseudo est déjà pris
    int player_index = find_player_by_pseudo(player->pseudo);
    if (player_index >= 0) {
        // Le pseudo existe, demander le mot de passe
        send(player->socket, "Entrez votre mot de passe : ", 28, 0);
        char password[MAX_PASSWORD_LENGTH];
        bytes_received = recv(player->socket, password, MAX_PASSWORD_LENGTH, 0);
        password[bytes_received] = '\0';

        // Vérifier le mot de passe
        if (strcmp(player_accounts[player_index].password, password) == 0) {
            send(player->socket, "Connexion réussie.\n", 19, 0);
        } else {
            send(player->socket, "Mot de passe incorrect.\n", 24, 0);
            close(player->socket);
            free(player);
            pthread_exit(NULL);
        }
    } else {
        // Le pseudo n'existe pas, créer un nouveau compte
        send(player->socket, "Pseudo non trouvé. Créez un mot de passe : ", 42, 0);
        char password[MAX_PASSWORD_LENGTH];
        bytes_received = recv(player->socket, password, MAX_PASSWORD_LENGTH, 0);
        password[bytes_received] = '\0';

        send(player->socket, "Entrez une courte biographie : ", 31, 0);
        char bio[MAX_BIO_LENGTH];
        bytes_received = recv(player->socket, bio, MAX_BIO_LENGTH, 0);
        bio[bytes_received] = '\0';

        if (create_new_player(player->pseudo, password, bio) == 0) {
            send(player->socket, "Compte créé et connecté.\n", 25, 0);
        } else {
            send(player->socket, "Erreur lors de la création du compte.\n", 38, 0);
            close(player->socket);
            free(player);
            pthread_exit(NULL);
        }
    }

    while (1) { // Boucle principale pour permettre au joueur de rejouer
        int reconnected = 0;
        // Vérifier si le joueur a une partie en cours
        for (int i = 0; i < num_disconnected; i++) {
            if (strcmp(disconnected_players[i].pseudo, player->pseudo) == 0) {
                // Reconnexion du joueur
                player->room_id = disconnected_players[i].room_id;
                player->player_id = disconnected_players[i].player_id;
                Room *room = disconnected_players[i].room;
                room->players[player->player_id] = player;
                room->players_connected++; 

                // Supprimer le joueur de la liste des déconnectés
                for (int j = i; j < num_disconnected - 1; j++) {
                    disconnected_players[j] = disconnected_players[j + 1];
                }
                num_disconnected--;

                send(player->socket, "RECONNECT", 9, 0);

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
        while (!challenge_accepted) {

            // Envoyer l'état des rooms
            snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
            for (int i = 0; i < num_rooms; i++) {
                char room_info[50];
                snprintf(room_info, sizeof(room_info), "Room %d: %d joueur(s) connecté(s)\n", i, rooms[i].players_connected);
                strcat(buffer, room_info);
            }
            send(player->socket, buffer, strlen(buffer), 0);

            // Recevoir la sélection de la room ou une commande
            bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                close(player->socket);
                free(player);
                pthread_exit(NULL);
            }

            buffer[bytes_received] = '\0';

            // Gérer la commande /defi
            if (strncmp(buffer, "/defi", 5) == 0) {

                char players_list[MAX_BUFFER_SIZE] = "Liste des joueurs connectés :\n";
                int players_found = 0;

                for (int i = 0; i < connected_players.count; i++) {
                    if (connected_players.players[i] != NULL) {
                        strcat(players_list, connected_players.players[i]->pseudo);
                        strcat(players_list, "\n");
                        players_found++;
                    }
                }


                // S'il n'y a pas de joueurs à défier
                if (players_found == 0) {
                    send(player->socket, "Aucun autre joueur n'est connecté.\n", 36, 0);
                    continue;
                }

                // Envoyer la liste des joueurs connectés
                send(player->socket, players_list, strlen(players_list), 0);

                // Demander au joueur de saisir le pseudo à défier
                send(player->socket, "Entrez le pseudo du joueur à défier : ", 39, 0);
                bytes_received = recv(player->socket, buffer, MAX_BUFFER_SIZE, 0);
                if (bytes_received <= 0) {
                    close(player->socket);
                    free(player);
                    pthread_exit(NULL);
                }
                buffer[bytes_received] = '\0';

                // Récupérer le pseudo du joueur défié
                char challenged_pseudo[MAX_PSEUDO_LENGTH];
                sscanf(buffer, "%s", challenged_pseudo);

                Player *challenged_player = NULL;
                
                for (int i = 0; i < connected_players.count; i++) {
                    if (connected_players.players[i] != NULL && strcmp(connected_players.players[i]->pseudo, challenged_pseudo) == 0) {
                        challenged_player = connected_players.players[i];
                        break;
                    }
                }

                if (challenged_player == NULL) {
                    send(player->socket, "Joueur introuvable ou non connecté.\n", 36, 0);
                    continue;  // Retourner à la sélection de room
                }

                send_challenge(player, challenged_player); 

                while (1) {
                    bytes_received = recv(challenged_player->socket, buffer, MAX_BUFFER_SIZE, 0);
                    if (bytes_received <= 0) {
                        close(player->socket);
                        free(player);
                        pthread_exit(NULL);
                    }
                    buffer[bytes_received] = '\0';

                    // NOE : ici c'est quand tu defie quelqu'un et que tu att sa rep ps la fonction handle_challenge_response sert plus a rien
                    if (strcmp(buffer, "/accepter") == 0) {
                        // handle_challenge_response(player, NULL, 1);
                        player->room_id = 0;
                        player->player_id = rooms[0].players_connected;
                        rooms[0].players[rooms[0].players_connected++] = player;
                        challenge_accepted = 1;
                        break; 
                    } else if (strcmp(buffer, "/refuser") == 0) {
                        // handle_challenge_response(player, NULL, 0);
                        player->room_id = 0;
                        player->player_id = rooms[0].players_connected;
                        rooms[0].players[rooms[0].players_connected++] = player;
                        continue;  
                    } else {
                        send(player->socket, "Attente de la réponse au défi...\n", 33, 0);
                    }
                }
            // NOE : ici c'est quand celui qui est defié fait /accepter
            // c'est ici qui faudrait forcé une action 
            } else if (strcmp(buffer, "/accepter") == 0) {
                
                player->room_id = 0;
                player->player_id = rooms[0].players_connected;
                rooms[0].players[rooms[0].players_connected++] = player;
                break;
                // NOE : verifie que le refuser marche aussi j'ai pas test
            } else if (strcmp(buffer, "/refuser") == 0) {
                // handle_challenge_response(NULL, player, 0);  // Refuser le défi
                player->room_id = 0;
                player->player_id = rooms[0].players_connected;
                rooms[0].players[rooms[0].players_connected++] = player;
            } else if (strncmp(buffer, "/bio", 4) == 0) {
                // Demander le pseudo du joueur pour lequel on veut voir la biographie
                send(player->socket, "Entrez le pseudonyme du joueur : ", 33, 0);
                
                // Recevoir le pseudo du joueur demandé
                bytes_received = recv(player->socket, buffer, MAX_PSEUDO_LENGTH, 0);
                if (bytes_received <= 0) {
                    close(player->socket);
                    free(player);
                    pthread_exit(NULL);
                }
                buffer[bytes_received] = '\0';

                // Rechercher le joueur par pseudo
                int player_index = find_player_by_pseudo(buffer);
                if (player_index >= 0) {
                    // Envoyer la biographie du joueur
                    char bio_message[MAX_BUFFER_SIZE];
                    snprintf(bio_message, sizeof(bio_message), "Biographie de %s : %s\n", player_accounts[player_index].pseudo, player_accounts[player_index].bio);
                    send(player->socket, bio_message, strlen(bio_message), 0);
                } else {
                    // Si le joueur n'existe pas
                    send(player->socket, "Joueur introuvable ou sans biographie.\n", 39, 0);
                }
            } else if (strcmp(buffer, "/liste") == 0) {
                send_connected_players_list(player->socket);

                // Attendre 5 secondes avant de réafficher le lobby
                sleep(5);

                // Réafficher l'état du lobby après le délai
                snprintf(buffer, sizeof(buffer), "ROOM_STATUS\n");
                for (int i = 0; i < num_rooms; i++) {
                    char room_info[50];
                    snprintf(room_info, sizeof(room_info), "Room %d: %d joueur(s) connecté(s)\n", i, rooms[i].players_connected);
                    strcat(buffer, room_info);
                }
                send(player->socket, buffer, strlen(buffer), 0);
            } else {
                // selection des rooms
                int selected_room = atoi(buffer);
                if (selected_room < 0 || selected_room >= num_rooms) {
                    send(player->socket, "INVALID_ROOM", 12, 0);
                    continue;
                }

                pthread_mutex_lock(&rooms[selected_room].room_mutex);

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

                if (!already_in_room) {
                    player->room_id = selected_room;
                    player->player_id = rooms[selected_room].players_connected;
                    rooms[selected_room].players[rooms[selected_room].players_connected++] = player;
                }

                pthread_mutex_unlock(&rooms[selected_room].room_mutex);

                send(player->socket, "JOINED_ROOM", 11, 0);
                break;
            }
        }
    }


        // NOE : c'est ici que le defieur att en attendant qu'une actoin u difier soit faite pour debloquer la situ
        // Attendre que les deux joueurs soient connectés
        while (rooms[player->room_id].players_connected < MAX_PLAYERS_PER_ROOM) {
            send(player->socket, "Joueur introuvable ou bit* connecté.\n", 36, 0);
            sleep(1);
        }

        send(player->socket, "GAME_START", 10, 0);

        // Boucle de jeu
        Room *room = &rooms[player->room_id];
        int opponent_id = 1 - player->player_id;
        Player *opponent;
        int game_over = 0;

        game_loop(player, opponent, room);
    }
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
        add_connected_player(new_player);
    }

    close(server_socket);
    return 0;
}
