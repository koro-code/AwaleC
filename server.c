#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define NB_CASES 6
#define NB_LIGNES 2
#define PORT 12345

typedef struct {
    int socket;
    int joueur_id;
} ClientInfo;

int plateau[NB_LIGNES][NB_CASES];
int scores[2] = {0, 0};
int joueur_actuel = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
ClientInfo clients[2];
int clients_connectes = 0;

void initialiser_plateau();
void distribuer_graines(int ligne, int case_index, int *score_joueur);
void *gerer_client(void *arg);
void envoyer_plateau(int socket);
int verifie_fin_jeu();
int mouvement_valide(int ligne, int choix_case);

int main() {
    int serveur_socket, client_socket;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Création du socket serveur
    if ((serveur_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(PORT);

    // Liaison du socket à l'adresse
    if (bind(serveur_socket, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) == -1) {
        perror("Erreur lors du bind");
        close(serveur_socket);
        exit(EXIT_FAILURE);
    }

    // Mise en écoute du socket
    if (listen(serveur_socket, 2) == -1) {
        perror("Erreur lors du listen");
        close(serveur_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente de connexions sur le port %d...\n", PORT);

    initialiser_plateau();

    // Accepter les connexions des deux clients
    while (clients_connectes < 2) {
        if ((client_socket = accept(serveur_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Erreur lors de l'acceptation d'une connexion");
            continue;
        }

        clients[clients_connectes].socket = client_socket;
        clients[clients_connectes].joueur_id = clients_connectes + 1;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, gerer_client, (void *)&clients[clients_connectes]) != 0) {
            perror("Erreur lors de la création du thread");
            close(client_socket);
        } else {
            pthread_detach(thread_id);
            clients_connectes++;
            printf("Joueur %d connecté.\n", clients[clients_connectes - 1].joueur_id);
        }
    }

    // Boucle principale du serveur
    while (1) {
        sleep(1);
        if (verifie_fin_jeu()) {
            printf("Le jeu est terminé.\n");
            break;
        }
    }

    // Fermeture des sockets
    for (int i = 0; i < 2; i++) {
        close(clients[i].socket);
    }
    close(serveur_socket);
    return 0;
}

void initialiser_plateau() {
    for (int i = 0; i < NB_LIGNES; i++) {
        for (int j = 0; j < NB_CASES; j++) {
            plateau[i][j] = 4;
        }
    }
}

void *gerer_client(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    int client_socket = client_info->socket;
    int joueur_id = client_info->joueur_id;
    int autre_joueur = (joueur_id == 1) ? 2 : 1;
    int ligne = (joueur_id == 1) ? 1 : 0;
    int fin_jeu = 0;

    // Envoyer le numéro du joueur
    send(client_socket, &joueur_id, sizeof(int), 0);

    while (!fin_jeu) {
        pthread_mutex_lock(&mutex);

        // Vérifier si c'est le tour du joueur
        if (joueur_actuel + 1 != joueur_id) {
            pthread_mutex_unlock(&mutex);
            sleep(1);
            continue;
        }

        // Envoyer le plateau au joueur
        envoyer_plateau(client_socket);

        // Recevoir le mouvement du joueur
        int choix_case;
        if (recv(client_socket, &choix_case, sizeof(int), 0) <= 0) {
            printf("Le joueur %d s'est déconnecté.\n", joueur_id);
            close(client_socket);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        choix_case--; // Indexation à partir de 0

        // Vérifier si le mouvement est valide
        if (!mouvement_valide(ligne, choix_case)) {
            int invalide = -1;
            send(client_socket, &invalide, sizeof(int), 0);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        // Distribuer les graines
        distribuer_graines(ligne, choix_case, &scores[joueur_id - 1]);

        // Vérifier la fin du jeu
        fin_jeu = verifie_fin_jeu();

        // Changer de joueur
        joueur_actuel = (joueur_actuel + 1) % 2;

        // Envoyer le plateau mis à jour aux deux joueurs
        for (int i = 0; i < 2; i++) {
            envoyer_plateau(clients[i].socket);
        }

        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void envoyer_plateau(int socket) {
    // Envoyer le plateau
    send(socket, plateau, sizeof(plateau), 0);

    // Envoyer les scores
    send(socket, scores, sizeof(scores), 0);

    // Envoyer le numéro du joueur actuel
    int joueur_envoyer = joueur_actuel + 1;
    send(socket, &joueur_envoyer, sizeof(int), 0);
}

int verifie_fin_jeu() {
    int graines_joueur[2] = {0, 0};

    for (int i = 0; i < NB_CASES; i++) {
        graines_joueur[0] += plateau[1][i]; // Joueur 1
        graines_joueur[1] += plateau[0][i]; // Joueur 2
    }

    if (graines_joueur[0] == 0 || graines_joueur[1] == 0) {
        printf("Fin du jeu. Score Joueur 1: %d, Score Joueur 2: %d\n", scores[0], scores[1]);
        return 1;
    }

    return 0;
}

int mouvement_valide(int ligne, int choix_case) {
    if (choix_case < 0 || choix_case >= NB_CASES) {
        return 0;
    }
    if (plateau[ligne][choix_case] == 0) {
        return 0;
    }
    return 1;
}

void distribuer_graines(int ligne, int case_index, int *score_joueur) {
    int graines = plateau[ligne][case_index];
    plateau[ligne][case_index] = 0;
    int l = ligne;
    int i = case_index;

    while (graines > 0) {
        // Avancer à la case suivante
        if (l == 1 && i == NB_CASES - 1) {
            l = 0; // Passer à la ligne du haut
        } else if (l == 0 && i == 0) {
            l = 1; // Passer à la ligne du bas
        } else if (l == 1) {
            i++;
        } else if (l == 0) {
            i--;
        }

        plateau[l][i]++;
        graines--;
    }

    // Gestion des captures
    while ((l == 1 - ligne) && (plateau[l][i] == 2 || plateau[l][i] == 3)) {
        *score_joueur += plateau[l][i];
        plateau[l][i] = 0;

        // Reculer à la case précédente
        if (l == 1 && i > 0) {
            i--;
        } else if (l == 0 && i < NB_CASES - 1) {
            i++;
        } else {
            break;
        }
    }
}
