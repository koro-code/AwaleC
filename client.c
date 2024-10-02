#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define NB_CASES 6
#define NB_LIGNES 2
#define PORT 12345

int plateau[NB_LIGNES][NB_CASES];
int scores[2];
int joueur_id;
int joueur_actuel;

void afficher_plateau();
void *recevoir_plateau(void *arg);

int main() {
    int client_socket;
    struct sockaddr_in serveur_addr;

    // Création du socket client
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(PORT);
    serveur_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresse du serveur (local)

    // Connexion au serveur
    if (connect(client_socket, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) == -1) {
        perror("Erreur lors de la connexion au serveur");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Recevoir le numéro du joueur
    recv(client_socket, &joueur_id, sizeof(int), 0);
    printf("Vous êtes le Joueur %d\n", joueur_id);

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, recevoir_plateau, (void *)&client_socket) != 0) {
        perror("Erreur lors de la création du thread");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Attendre que ce soit le tour du joueur
        if (joueur_actuel != joueur_id) {
            sleep(1);
            continue;
        }

        afficher_plateau();

        printf("C'est votre tour. Choisissez une case à jouer (1-6): ");
        int choix_case;
        scanf("%d", &choix_case);

        // Envoyer le mouvement au serveur
        send(client_socket, &choix_case, sizeof(int), 0);

        // Vérifier si le mouvement était invalide
        int validation;
        recv(client_socket, &validation, sizeof(int), MSG_DONTWAIT);
        if (validation == -1) {
            printf("Mouvement invalide. Veuillez réessayer.\n");
            continue;
        }
    }

    close(client_socket);
    return 0;
}

void *recevoir_plateau(void *arg) {
    int socket = *(int *)arg;

    while (1) {
        // Recevoir le plateau
        if (recv(socket, plateau, sizeof(plateau), 0) <= 0) {
            printf("Connexion perdue avec le serveur.\n");
            close(socket);
            exit(EXIT_FAILURE);
        }

        // Recevoir les scores
        recv(socket, scores, sizeof(scores), 0);

        // Recevoir le numéro du joueur actuel
        recv(socket, &joueur_actuel, sizeof(int), 0);

        afficher_plateau();

        // Vérifier la fin du jeu
        if (joueur_actuel == -1) {
            printf("Le jeu est terminé.\n");
            printf("Score final - Joueur 1 : %d, Joueur 2 : %d\n", scores[0], scores[1]);

            if (scores[0] > scores[1]) {
                printf("Le Joueur 1 gagne !\n");
            } else if (scores[1] > scores[0]) {
                printf("Le Joueur 2 gagne !\n");
            } else {
                printf("Match nul !\n");
            }
            close(socket);
            exit(0);
        }
    }

    pthread_exit(NULL);
}

void afficher_plateau() {
    system("clear");
    int largeur_case = 3; // Ajuster si les nombres dépassent deux chiffres
    int i, j;

    // Affichage des numéros de cases en haut
    printf("\t");
    for (i = 0; i < NB_CASES; i++) {
        printf("[%d]   ", i + 1);
    }
    printf("\n");

    // Ligne supérieure
    printf(" +----");
    for (i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+----+\n");

    // Première ligne de graines (ligne du haut)
    printf(" |    ");
    for (i = 0; i < NB_CASES; i++) {
        printf("| %3d ", plateau[0][i]);
    }
    printf("|    |\n");

    // Ligne centrale avec les scores des joueurs
    printf(" | %2d ", scores[1]);
    for (i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+ %2d |\n", scores[0]);

    // Deuxième ligne de graines (ligne du bas)
    printf(" |    ");
    for (i = 0; i < NB_CASES; i++) {
        printf("| %3d ", plateau[1][i]);
    }
    printf("|    |\n");

    // Ligne inférieure
    printf(" +----");
    for (i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+----+\n");

    // Affichage des numéros de cases en bas
    printf("\t");
    for (i = 0; i < NB_CASES; i++) {
        printf("[%d]   ", i + 1);
    }
    printf("\n\n");

    printf("Score Joueur 1: %d\tScore Joueur 2: %d\n", scores[0], scores[1]);
    printf("C'est le tour du Joueur %d\n", joueur_actuel);
}
