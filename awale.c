#include <stdio.h>
#include <stdlib.h>

#define NB_CASES 6
#define NB_LIGNES 2

void initialiser_plateau(int plateau[NB_LIGNES][NB_CASES]);
void afficher_plateau(int plateau[NB_LIGNES][NB_CASES], int score_j1, int score_j2);
int tour_joueur(int plateau[NB_LIGNES][NB_CASES], int joueur, int *score_j1, int *score_j2);
void distribuer_graines(int plateau[NB_LIGNES][NB_CASES], int ligne, int case_index, int *score_joueur);
int verifie_fin_jeu(int plateau[NB_LIGNES][NB_CASES], int joueur);

int main() {
    int plateau[NB_LIGNES][NB_CASES];
    int joueur = 0; // 0 pour joueur 1, 1 pour joueur 2
    int fin_jeu = 0;
    int score_j1 = 0, score_j2 = 0;

    initialiser_plateau(plateau);

    while(!fin_jeu) {
        afficher_plateau(plateau, score_j1, score_j2);
        printf("Joueur %d, c'est votre tour.\n", joueur + 1);
        if(!tour_joueur(plateau, joueur == 0 ? 1 : 2, &score_j1, &score_j2)) {
            printf("Aucun mouvement possible pour le joueur %d. Le jeu se termine.\n", joueur + 1);
            break;
        }
        fin_jeu = verifie_fin_jeu(plateau, joueur);
        joueur = 1 - joueur; // Changer de joueur
    }

    afficher_plateau(plateau, score_j1, score_j2);
    printf("Le jeu est terminé.\n");
    printf("Score final - Joueur 1 : %d, Joueur 2 : %d\n", score_j1, score_j2);

    if(score_j1 > score_j2) {
        printf("Le Joueur 1 gagne !\n");
    } else if(score_j2 > score_j1) {
        printf("Le Joueur 2 gagne !\n");
    } else {
        printf("Match nul !\n");
    }

    return 0;
}

void initialiser_plateau(int plateau[NB_LIGNES][NB_CASES]) {
    for(int i = 0; i < NB_LIGNES; i++) {
        for(int j = 0; j < NB_CASES; j++) {
            plateau[i][j] = 4;
        }
    }
}

void afficher_plateau(int plateau[NB_LIGNES][NB_CASES], int score_j1, int score_j2) {
    int largeur_case = 3; // Ajuster si les nombres dépassent deux chiffres
    int i, j;

    // Calculer la largeur nécessaire pour les cases
    int max_val = 0;
    for(i = 0; i < NB_LIGNES; i++) {
        for(j = 0; j < NB_CASES; j++) {
            if(plateau[i][j] > max_val) {
                max_val = plateau[i][j];
            }
        }
    }
    while(max_val >= 10) {
        largeur_case++;
        max_val /= 10;
    }

    // Affichage des numéros de cases en haut
    printf("\t");
    for(i = 0; i < NB_CASES; i++) {
        printf("[%d]   ", i + 1);
    }
    printf("\n");

    // Ligne supérieure
    printf(" +----");
    for(i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+----+\n");

    // Première ligne de graines (ligne du haut)
    printf(" |    ");
    for(i = 0; i < NB_CASES; i++) {
        printf("| %3d ", plateau[0][i]);
    }
    printf("|    |\n");

    // Ligne centrale avec les scores des joueurs
    printf(" | %2d ", score_j2);
    for(i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+ %2d |\n", score_j1);

    // Deuxième ligne de graines (ligne du bas)
    printf(" |    ");
    for(i = 0; i < NB_CASES; i++) {
        printf("| %3d ", plateau[1][i]);
    }
    printf("|    |\n");

    // Ligne inférieure
    printf(" +----");
    for(i = 0; i < NB_CASES; i++) {
        printf("+-----");
    }
    printf("+----+\n");

    // Affichage des numéros de cases en bas
    printf("\t");
    for(i = 0; i < NB_CASES; i++) {
        printf("[%d]   ", i + 1);
    }
    printf("\n\n");
}

int tour_joueur(int plateau[NB_LIGNES][NB_CASES], int joueur, int *score_j1, int *score_j2) {
    int ligne = joueur == 1 ? 1 : 0;
    int choix_case;
    int graine_dispo = 0;

    // Vérifier s'il y a des mouvements possibles
    for(int i = 0; i < NB_CASES; i++) {
        if(plateau[ligne][i] > 0) {
            graine_dispo = 1;
            break;
        }
    }
    if(!graine_dispo) {
        return 0; // Pas de mouvement possible
    }

    do {
        printf("Joueur %d, choisissez une case à jouer (1-%d): ", joueur, NB_CASES);
        scanf("%d", &choix_case);
        choix_case--; // Indexation à partir de 0

        if(choix_case < 0 || choix_case >= NB_CASES || plateau[ligne][choix_case] == 0) {
            printf("Choix invalide. Veuillez réessayer.\n");
        } else {
            // Vérifier si le coup ne prive pas l'adversaire de toutes ses graines
            int total_graines_adversaire = 0;
            int ligne_adversaire = 1 - ligne;
            for(int i = 0; i < NB_CASES; i++) {
                total_graines_adversaire += plateau[ligne_adversaire][i];
            }

            if(total_graines_adversaire == 0) {
                printf("Vous ne pouvez pas priver l'adversaire de toutes ses graines.\n");
            } else {
                break;
            }
        }
    } while(1);

    distribuer_graines(plateau, ligne, choix_case, joueur == 1 ? score_j1 : score_j2);
    return 1;
}

void distribuer_graines(int plateau[NB_LIGNES][NB_CASES], int ligne, int case_index, int *score_joueur) {
    int graines = plateau[ligne][case_index];
    plateau[ligne][case_index] = 0;
    int l = ligne;
    int i = case_index;

    while(graines > 0) {
        // Avancer à la case suivante
        if(l == 1 && i == NB_CASES - 1) {
            l = 0; // Passer à la ligne du haut
        } else if(l == 0 && i == 0) {
            l = 1; // Passer à la ligne du bas
        } else if(l == 1) {
            i++;
        } else if(l == 0) {
            i--;
        }

        // Sauter la case de départ si on fait un tour complet
        if(graines == 12 && i == case_index && l == ligne) {
            continue;
        }

        plateau[l][i]++;
        graines--;
    }

    // Gestion des captures
    while((l == 1 - ligne) && (plateau[l][i] == 2 || plateau[l][i] == 3)) {
        *score_joueur += plateau[l][i];
        plateau[l][i] = 0;

        // Reculer à la case précédente
        if(l == 1 && i > 0) {
            i--;
        } else if(l == 0 && i < NB_CASES - 1) {
            i++;
        } else {
            break;
        }
    }
}

int verifie_fin_jeu(int plateau[NB_LIGNES][NB_CASES], int joueur) {
    int graine_joueur = 0;
    int ligne = joueur == 0 ? 1 : 0;

    for(int i = 0; i < NB_CASES; i++) {
        graine_joueur += plateau[ligne][i];
    }

    if(graine_joueur == 0) {
        return 1; // Le jeu se termine si un joueur n'a plus de graines sur son côté
    }
    return 0;
}
