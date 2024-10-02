#include "client.h"

void *receive_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[MAX_BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(sock, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';

        // Clear the terminal
        system("clear");

        // Handle different server messages
        if (strncmp(buffer, "ROOM_STATUS", 11) == 0) {
            printf("+------------------------------------------------------------------+\n");
            printf("|   Voici les rooms disponibles :                                   |\n");
            printf("|                                                                  |\n");
            char *room_info = buffer + 12;
            char *line = strtok(room_info, "\n");
            while (line != NULL) {
                printf("|  --------------------------------------------------------------  |\n");
                printf("|  | %s |  |\n", line);
                line = strtok(NULL, "\n");
            }
            printf("|                                                                  |\n");
            printf("+------------------------------------------------------------------+\n");
            printf("Veuillez sélectionner une room de jeu : ");
            fflush(stdout);
        } else if (strcmp(buffer, "INVALID_ROOM") == 0) {
            printf("/!\\ La room que vous avez choisie est invalide, choisissez-en une autre /!\\\n");
            printf("Veuillez sélectionner une room de jeu : ");
            fflush(stdout);
        } else if (strcmp(buffer, "ROOM_FULL") == 0) {
            printf("/!\\ La room que vous avez choisie est déjà complète, choisissez-en une autre /!\\\n");
            printf("Veuillez sélectionner une room de jeu : ");
            fflush(stdout);
        } else if (strcmp(buffer, "JOINED_ROOM") == 0) {
            printf("Parfait, vous allez être connecté à votre room !\n");
            printf("En attente d'un autre joueur...\n");
        } else if (strcmp(buffer, "GAME_START") == 0) {
            printf("La partie commence !\n");
        } else if (strncmp(buffer, "YOUR_TURN", 9) == 0) {
            printf("%s", buffer + 9);
            printf("\nEntrez votre coup : ");
            fflush(stdout);
        } else if (strncmp(buffer, "WAITING_FOR_OPPONENT", 20) == 0) {
            printf("En attente du coup de votre adversaire...\n");
        } else if (strncmp(buffer, "BOARD_STATE", 11) == 0) {
            printf("%s", buffer + 11);
        } else if (strcmp(buffer, "INVALID_MOVE") == 0) {
            printf("Mouvement invalide. Veuillez réessayer.\n");
            printf("Entrez votre coup : ");
            fflush(stdout);
        } else if (strncmp(buffer, "GAME_OVER", 9) == 0) {
            printf("%s\n", buffer + 9);
            exit(EXIT_SUCCESS);
        } else if (strcmp(buffer, "OPPONENT_DISCONNECTED") == 0) {
            printf("Votre adversaire s'est déconnecté. Vous gagnez par défaut !\n");
            exit(EXIT_SUCCESS);
        } else {
            printf("%s\n", buffer);
        }
    }

    if (bytes_received == 0) {
        printf("Disconnected from server.\n");
    } else {
        perror("recv failed");
    }

    exit(EXIT_FAILURE);
    return 0;
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char pseudo[MAX_PSEUDO_LENGTH];
    char buffer[MAX_BUFFER_SIZE];
    pthread_t recv_thread;

    printf("Enter your pseudonym (max %d characters): ", MAX_PSEUDO_LENGTH);
    fgets(pseudo, MAX_PSEUDO_LENGTH, stdin);
    pseudo[strcspn(pseudo, "\n")] = '\0';

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // Replace "127.0.0.1" with the server's IP if necessary
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Send pseudonym to server
    send(sock, pseudo, strlen(pseudo), 0);

    // Create thread to receive messages
    if (pthread_create(&recv_thread, NULL, receive_handler, (void *)&sock) != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Handle user input
    while (1) {
        fgets(buffer, MAX_BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strlen(buffer) > 0) {
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    close(sock);
    return 0;
}
