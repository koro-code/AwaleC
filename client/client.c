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
            printf("Available Rooms:\n%s\n", buffer + 12);
            printf("Please select a room number: ");
            fflush(stdout);
        } else if (strcmp(buffer, "INVALID_ROOM") == 0) {
            printf("/!\\ The room you selected is invalid, please choose another /!\\\n");
            printf("Please select a room number: ");
            fflush(stdout);
        } else if (strcmp(buffer, "ROOM_FULL") == 0) {
            printf("/!\\ The room you selected is already full, please choose another /!\\\n");
            printf("Please select a room number: ");
            fflush(stdout);
        } else if (strcmp(buffer, "JOINED_ROOM") == 0) {
            printf("Perfect, you will be connected to your room!\n");
            printf("Waiting for an opponent...\n");
        } else if (strcmp(buffer, "GAME_START") == 0) {
            printf("Game is starting!\n");
        } else if (strncmp(buffer, "YOUR_TURN", 9) == 0) {
            printf("%s\n", buffer + 10);
            printf("Enter your move: ");
            fflush(stdout);
        } else if (strncmp(buffer, "WAITING_FOR_OPPONENT", 20) == 0) {
            printf("Waiting for opponent's move...\n");
        } else if (strncmp(buffer, "BOARD_UPDATE", 12) == 0) {
            printf("%s\n", buffer + 13);
        } else if (strcmp(buffer, "OPPONENT_DISCONNECTED") == 0) {
            printf("Your opponent has disconnected. You win by default!\n");
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

    if (pthread_create(&recv_thread, NULL, receive_handler, (void *)&sock) != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        fgets(buffer, MAX_BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}
