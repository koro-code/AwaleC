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