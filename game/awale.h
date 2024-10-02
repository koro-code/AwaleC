#include <stdio.h>
#include <stdlib.h>

#define NB_CASES 6
#define NB_LIGNES 2

void initialiser_plateau(int plateau[NB_LIGNES][NB_CASES]);
void afficher_plateau(int plateau[NB_LIGNES][NB_CASES], int score_j1, int score_j2);
int tour_joueur(int plateau[NB_LIGNES][NB_CASES], int joueur, int *score_j1, int *score_j2);
void distribuer_graines(int plateau[NB_LIGNES][NB_CASES], int ligne, int case_index, int *score_joueur);
int verifie_fin_jeu(int plateau[NB_LIGNES][NB_CASES], int joueur);
