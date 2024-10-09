// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_BUFFER_SIZE 2048
#define MAX_PSEUDO_LENGTH 50

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char pseudo[MAX_PSEUDO_LENGTH];
    char send_buffer[MAX_BUFFER_SIZE];
    char recv_buffer[MAX_BUFFER_SIZE];
    char *server_ip;
    fd_set read_fds;
    int max_fd;
    int in_chat_mode = 0; // Indique si le client est en mode chat

    // Utiliser l'IP fournie en argument, sinon 127.0.0.1 par défaut
    if (argc > 1) {
        server_ip = argv[1];
    } else {
        server_ip = "127.0.0.1";
    }

    printf("Entrez votre pseudonyme (max %d caractères): ", MAX_PSEUDO_LENGTH - 1);
    fgets(pseudo, MAX_PSEUDO_LENGTH, stdin);
    pseudo[strcspn(pseudo, "\n")] = '\0';

    // Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Échec de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Conversion de l'IP du serveur en format réseau
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        exit(EXIT_FAILURE);
    }

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Échec de la connexion");
        exit(EXIT_FAILURE);
    }

    // Envoi du pseudonyme au serveur
    send(sock, pseudo, strlen(pseudo), 0);

    // Boucle principale utilisant select()
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Erreur avec select");
            break;
        }

        // Vérifier si le socket est prêt pour la lecture (messages du serveur)
        if (FD_ISSET(sock, &read_fds)) {
            int bytes_received = recv(sock, recv_buffer, MAX_BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("Le serveur s'est déconnecté.\n");
                break;
            }
            recv_buffer[bytes_received] = '\0';

            // Traitement du message reçu
            if (strncmp(recv_buffer, "GAME_OVER", 9) == 0) {
                printf("%s\n", recv_buffer + 9);
                printf("Voulez-vous rejouer ? (oui/non): ");
                fflush(stdout);
            } else if (strncmp(recv_buffer, "SCOREBOARD", 10) == 0) {
                printf("Tableau des scores :\n");
                printf("%s\n", recv_buffer + 11);
            } else if (strncmp(recv_buffer, "ROOM_STATUS", 11) == 0) {
                // Effacer le terminal
                system("clear");

                printf("+------------------------------------------------------------------+\n");
                printf("|   Voici les rooms disponibles :                                  |\n");
                printf("|                                                                  |\n");
                char *room_info = recv_buffer + 12;
                char *line = strtok(room_info, "\n");
                while (line != NULL) {
                    printf("|  --------------------------------------------------------------  |\n");
                    printf("|  | %s | \t\t\t\t   |\n", line);
                    printf("|  --------------------------------------------------------------  |\n");
                    printf("|                                                                  |\n");
                    line = strtok(NULL, "\n");
                }
                printf("|                                                                  |\n");
                printf("+------------------------------------------------------------------+\n");
                printf("Veuillez sélectionner une room de jeu : ");
                fflush(stdout);
            } else if (strcmp(recv_buffer, "INVALID_ROOM") == 0) {
                printf("/!\\ La room que vous avez choisie est invalide, choisissez-en une autre /!\\\n");
                printf("Veuillez sélectionner une room de jeu : ");
                fflush(stdout);
            } else if (strcmp(recv_buffer, "KEEP_ALIVE") == 0) {
                // Ne rien faire, on ignore ce message
            } else if (strcmp(recv_buffer, "ROOM_FULL") == 0) {
                printf("/!\\ La room que vous avez choisie est déjà complète, choisissez-en une autre /!\\\n");
                printf("Veuillez sélectionner une room de jeu : ");
                fflush(stdout);
            } else if (strcmp(recv_buffer, "JOINED_ROOM") == 0) {
                printf("Parfait, vous allez être connecté à votre room !\n");
                printf("En attente d'un autre joueur...\n");
            } else if (strcmp(recv_buffer, "GAME_START") == 0) {
                printf("La partie commence !\n");
            } else if (strcmp(recv_buffer, "RECONNECT") == 0) {
                printf("Reconnexion à votre partie en cours...\n");
            } else if (strcmp(recv_buffer, "OPPONENT_RECONNECTED") == 0) {
                printf("Votre adversaire s'est reconnecté. La partie reprend.\n");
            } else if (strncmp(recv_buffer, "CLEAR_SCREEN", 12) == 0) {
                system("clear");
                // Trouver la position de "BOARD_STATE\n"
                char *board_state = strstr(recv_buffer, "BOARD_STATE\n");
                if (board_state != NULL) {
                    printf("%s", board_state + 11); // Afficher l'état du plateau
                }
            } else if (strncmp(recv_buffer, "BOARD_STATE", 11) == 0) {
                printf("%s", recv_buffer + 11);
            } else if (strncmp(recv_buffer, "YOUR_TURN", 9) == 0) {
                printf("%s", recv_buffer + 9);
                printf("\nEntrez votre coup : ");
                fflush(stdout);
            } else if (strncmp(recv_buffer, "WAITING_FOR_OPPONENT", 20) == 0) {
                printf("En attente du coup de votre adversaire...\n");
            } else if (strcmp(recv_buffer, "INVALID_MOVE") == 0) {
                printf("Mouvement invalide. Veuillez réessayer.\n");
                printf("Entrez votre coup : ");
                fflush(stdout);
            } else if (strcmp(recv_buffer, "ENTER_CHAT_MODE") == 0) {
                in_chat_mode = 1;
                system("clear");
                printf("Vous êtes maintenant en mode chat. Tapez '/game' pour revenir au jeu.\n");
            } else if (strcmp(recv_buffer, "ENTER_GAME_MODE") == 0) {
                in_chat_mode = 0;
                system("clear");
                printf("Vous êtes maintenant en mode jeu. Tapez '/chat' pour accéder au chat.\n");
            } else if (strncmp(recv_buffer, "CHAT_HISTORY", 12) == 0) {
                // Afficher l'historique du chat
                printf("%s\n", recv_buffer + 13);
            } else if (strncmp(recv_buffer, "CHAT_UPDATE", 11) == 0) {
                // Afficher le nouveau message de chat
                printf("%s\n", recv_buffer + 12);
            } else {
                printf("%s\n", recv_buffer);
            }
        }

        // Vérifier si l'entrée standard est prête (entrée utilisateur)
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(send_buffer, MAX_BUFFER_SIZE, stdin) != NULL) {
                send_buffer[strcspn(send_buffer, "\n")] = '\0';
                if (strlen(send_buffer) > 0) {
                    if (strcmp(send_buffer, "exit") == 0 || strcmp(send_buffer, "disconnect") == 0 || strcmp(send_buffer, "non") == 0) {
                        printf("Déconnexion...\n");
                        close(sock);
                        exit(EXIT_SUCCESS);
                    } else if (strcmp(send_buffer, "oui") == 0) {
                        // Envoyer la réponse au serveur
                        send(sock, send_buffer, strlen(send_buffer), 0);
                        in_chat_mode = 0; // Réinitialiser le mode chat
                        continue;
                    }
                    if (strcmp(send_buffer, "/chat") == 0 || strcmp(send_buffer, "/game") == 0 || strcmp(send_buffer, "/score") == 0) {
                        send(sock, send_buffer, strlen(send_buffer), 0);
                        continue;
                    }
                    if (in_chat_mode) {
                        // Envoyer le message de chat au serveur
                        send(sock, send_buffer, strlen(send_buffer), 0);
                    } else {
                        // Envoyer le message de jeu au serveur
                        send(sock, send_buffer, strlen(send_buffer), 0);
                    }
                }
            }
        }
    }

    close(sock);
    return 0;
}
