#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define TEMPS_ATTENTE 1
#define CYCLE_FEUX 5

typedef struct {
    int x;
    int y;
    char etat;
    char direction; 
    pthread_mutex_t mutex;
} FeuSignalisation;

typedef struct {
    FeuSignalisation *feuH;
    FeuSignalisation *feuV;
    char **grille;
    pthread_mutex_t *mutex_grille;
} PaireFeux;


typedef struct {
    char **grille;
    int hauteur;
    int largeur;
    int x;
    int y;
    char direction;
    pthread_mutex_t *mutex_grille;
    PaireFeux **paires_feux;
    int nb_paires;
} ThreadParams;

pthread_mutex_t mutex_grille = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t running = 1;

void handle_sigint(int sig) {
    running = 0;
}

void afficherGrille(char **grille, int hauteur, int largeur) {
    pthread_mutex_lock(&mutex_grille);
    system("clear");
    for (int i = 0; i < hauteur; i++) {
        for (int j = 0; j < largeur; j++) {
            printf("%c", grille[i][j]);
        }
        printf("\n");
    }
    printf("\nAppuyez sur Ctrl+C pour quitter.\n");
    pthread_mutex_unlock(&mutex_grille);
}

int verifierFeu(ThreadParams *params) {
    for (int i = 0; i < params->nb_paires; i++) {
        FeuSignalisation *feuH = params->paires_feux[i]->feuH;
        FeuSignalisation *feuV = params->paires_feux[i]->feuV;

        if (params->direction == 'H') {
            pthread_mutex_lock(&feuH->mutex);
            if (params->x == feuH->x + 1 && params->y == feuH->y - 1) {
                char etat = feuH->etat;
                pthread_mutex_unlock(&feuH->mutex);
                return etat == 'V'; 
            }
            pthread_mutex_unlock(&feuH->mutex);
        }

        else if (params->direction == 'V') {
            pthread_mutex_lock(&feuV->mutex);
            if (params->x == feuV->x - 1 && params->y == feuV->y + 1) {
                char etat = feuV->etat;
                pthread_mutex_unlock(&feuV->mutex);
                return etat == 'V'; 
            }
            pthread_mutex_unlock(&feuV->mutex);
        }
    }
    return 1; 
}


void *deplacerVoiture(void *arg) {
    ThreadParams *params = (ThreadParams *)arg;
    while (running) {
        pthread_mutex_lock(params->mutex_grille);
        

        int next_x = params->x;
        int next_y = params->y;
        
        if (params->direction == 'H') {
            next_y++;
        } else {
            next_x++;
        }


        if (next_x >= params->hauteur || next_y >= params->largeur) {
            pthread_mutex_unlock(params->mutex_grille);
            return NULL;
        }

        if (params->grille[next_x][next_y] == '*') {
            pthread_mutex_unlock(params->mutex_grille);
            usleep(500000); 
            continue;
        }


        if (!verifierFeu(params)) {
            pthread_mutex_unlock(params->mutex_grille);
            usleep(500000);
            continue;
        }

        int old_x = params->x;
        int old_y = params->y;

        params->x = next_x;
        params->y = next_y;

        params->grille[old_x][old_y] = (params->direction == 'H') ? '-' : '|';
        params->grille[params->x][params->y] = '*';

        pthread_mutex_unlock(params->mutex_grille);
        usleep(TEMPS_ATTENTE * 1000000);
    }
    return NULL;
}


void *gererPaireFeux(void *arg) {
    PaireFeux *paire = (PaireFeux *)arg;
    while (running) {
        pthread_mutex_lock(&paire->feuH->mutex);
        pthread_mutex_lock(&paire->feuV->mutex);

        if (paire->feuH->etat == 'R') {
            paire->feuH->etat = 'V';
            paire->feuV->etat = 'R';
        } else {
            paire->feuH->etat = 'R';
            paire->feuV->etat = 'V';
        }

        pthread_mutex_lock(paire->mutex_grille);
        paire->grille[paire->feuH->x][paire->feuH->y] = paire->feuH->etat;
        paire->grille[paire->feuV->x][paire->feuV->y] = paire->feuV->etat;
        pthread_mutex_unlock(paire->mutex_grille);

        pthread_mutex_unlock(&paire->feuH->mutex);
        pthread_mutex_unlock(&paire->feuV->mutex);
        
        sleep(CYCLE_FEUX);
    }
    return NULL;
}


int main() {
    signal(SIGINT, handle_sigint);

    const char *nomFichier = "nombres.txt";
    FILE *fichier = fopen(nomFichier, "r");
    if (!fichier) {
        perror("Erreur lors de l'ouverture du fichier");
        return EXIT_FAILURE;
    }

    int hauteur, largeur, nombreVehicules, nombreRoutesH, nombreRoutesV;
    if (fscanf(fichier, "%d %d\n%d\n%d %d", &hauteur, &largeur, &nombreVehicules, 
               &nombreRoutesH, &nombreRoutesV) != 5) {
        fprintf(stderr, "Erreur : fichier mal formé.\n");
        fclose(fichier);
        return EXIT_FAILURE;
    }
    fclose(fichier);

    char **grille = (char **)malloc(hauteur * sizeof(char *));
    for (int i = 0; i < hauteur; i++) {
        grille[i] = (char *)malloc(largeur * sizeof(char));
        for (int j = 0; j < largeur; j++) {
            grille[i][j] = ' ';
        }
    }

    int espacementH = hauteur / (nombreRoutesH + 1);
    int espacementV = largeur / (nombreRoutesV + 1);
    
    for (int i = 0; i < nombreRoutesH; i++) {
        int y = (i + 1) * espacementH;
        for (int x = 0; x < largeur; x++) {
            grille[y][x] = '-';
        }
    }
    
    for (int i = 0; i < nombreRoutesV; i++) {
        int x = (i + 1) * espacementV;
        for (int y = 0; y < hauteur; y++) {
            grille[y][x] = '|';
        }
    }

    int nb_paires = nombreRoutesH * nombreRoutesV;
    PaireFeux **paires_feux = malloc(nb_paires * sizeof(PaireFeux*));
    pthread_t *threadsFeux = malloc(nb_paires * sizeof(pthread_t));
    
    int paire_index = 0;
    for (int i = 0; i < nombreRoutesH; i++) {
    for (int j = 0; j < nombreRoutesV; j++) {
        paires_feux[paire_index] = malloc(sizeof(PaireFeux));
        
        FeuSignalisation *feuH = malloc(sizeof(FeuSignalisation));
        feuH->x = (i + 1) * espacementH - 1;
        feuH->y = (j + 1) * espacementV + 1;
        feuH->etat = 'R';
        feuH->direction = 'H';
        pthread_mutex_init(&feuH->mutex, NULL);
        grille[feuH->x][feuH->y] = feuH->etat;
        
        FeuSignalisation *feuV = malloc(sizeof(FeuSignalisation));
        feuV->x = (i + 1) * espacementH + 1;
        feuV->y = (j + 1) * espacementV - 1;
        feuV->etat = 'V';
        feuV->direction = 'V';
        pthread_mutex_init(&feuV->mutex, NULL);
        grille[feuV->x][feuV->y] = feuV->etat;
        
        paires_feux[paire_index]->feuH = feuH;
        paires_feux[paire_index]->feuV = feuV;
        paires_feux[paire_index]->grille = grille;
        paires_feux[paire_index]->mutex_grille = &mutex_grille;

        pthread_create(&threadsFeux[paire_index], NULL, gererPaireFeux, paires_feux[paire_index]);
        paire_index++;
    }
}


    srand(time(NULL));
    pthread_t *threadsVehicules = malloc(nombreVehicules * sizeof(pthread_t));
    ThreadParams *params = malloc(nombreVehicules * sizeof(ThreadParams));

    for (int v = 0; v < nombreVehicules; v++) {
        int placed = 0;
        while (!placed) {
            int orientation = rand() % 2;
            if (orientation == 0) {
                int route = rand() % nombreRoutesH;
                int y = (route + 1) * espacementH;
                if (grille[y][0] == '-') {
                    grille[y][0] = '*';
                    params[v] = (ThreadParams){
                        .grille = grille,
                        .hauteur = hauteur,
                        .largeur = largeur,
                        .x = y,
                        .y = 0,
                        .direction = 'H',
                        .mutex_grille = &mutex_grille,
                        .paires_feux = paires_feux,
                        .nb_paires = nb_paires
                    };
                    placed = 1;
                }
            } else {
                int route = rand() % nombreRoutesV;
                int x = (route + 1) * espacementV;
                if (grille[0][x] == '|') {
                    grille[0][x] = '*';
                    params[v] = (ThreadParams){
                        .grille = grille,
                        .hauteur = hauteur,
                        .largeur = largeur,
                        .x = 0,
                        .y = x,
                        .direction = 'V',
                        .mutex_grille = &mutex_grille,
                        .paires_feux = paires_feux,
                        .nb_paires = nb_paires
                    };
                    placed = 1;
                }
            }
        }
        pthread_create(&threadsVehicules[v], NULL, deplacerVoiture, &params[v]);
        sleep(1);
    }

    while (running) {
        afficherGrille(grille, hauteur, largeur);
        usleep(100000);
    }

    for (int v = 0; v < nombreVehicules; v++) {
        pthread_cancel(threadsVehicules[v]);
    }
    
    for (int p = 0; p < nb_paires; p++) {
        pthread_cancel(threadsFeux[p]);
        pthread_mutex_destroy(&paires_feux[p]->feuH->mutex);
        pthread_mutex_destroy(&paires_feux[p]->feuV->mutex);
        free(paires_feux[p]->feuH);
        free(paires_feux[p]->feuV);
        free(paires_feux[p]);
    }
    
    free(threadsFeux);
    free(threadsVehicules);
    free(params);
    free(paires_feux);
    
    for (int i = 0; i < hauteur; i++) {
        free(grille[i]);
    }
    free(grille);
    
    pthread_mutex_destroy(&mutex_grille);

    printf("\nSimulation terminée\n");
    return EXIT_SUCCESS;
}