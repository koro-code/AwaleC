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