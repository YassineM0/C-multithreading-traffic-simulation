#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define TEMPS_ATTENTE 1
volatile sig_atomic_t running = 1;

typedef struct {
    char **grille;
    int hauteur;
    int largeur;
    int x;
    int y;
    char direction;
    pthread_mutex_t *mutex;
} ThreadParams;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void afficherGrille(char **grille, int hauteur, int largeur);
void *deplacerVoiture(void *arg);

void handle_sigint(int sig) {
    running = 0;
}

int main() {
    signal(SIGINT, handle_sigint);
    
    const char *nomFichier = "nombres.txt";
    FILE *fichier = fopen(nomFichier, "r");
    
    if (fichier == NULL) {
        perror("erreur lors de l'ouverture des fichiers");
        return EXIT_FAILURE;
    }

    int hauteur, largeur, nombreVehicules, nombreRoutesH, nombreRoutesV;
    
    if (fscanf(fichier, "%d %d %d %d %d", &hauteur, &largeur, &nombreVehicules, &nombreRoutesH, &nombreRoutesV) != 5) {
        fprintf(stderr, "le fichier ne contient pas assez de données");
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

    int routeHorizontale = hauteur / 2;
    int routeVerticale = largeur / 2;

    for (int j = 0; j < largeur; j++) {
        grille[routeHorizontale][j] = '-';
    }
    for (int i = 0; i < hauteur; i++) {
        grille[i][routeVerticale] = '|';
    }

    srand(time(NULL));
    pthread_t threads[nombreVehicules];
    ThreadParams params[nombreVehicules];

    for (int v = 0; v < nombreVehicules; v++) {
        int placed = 0;
        while (!placed) {
            int orientation = rand() % 2;
            if (orientation == 0) {
                int position = rand() % largeur;
                if (grille[routeHorizontale][position] == '-') {
                    grille[routeHorizontale][position] = '*';
                    params[v] = (ThreadParams){grille, hauteur, largeur, routeHorizontale, position, 'H', &mutex};
                    placed = 1;
                }
            } else {
                int position = rand() % hauteur;
                if (grille[position][routeVerticale] == '|') {
                    grille[position][routeVerticale] = '*';
                    params[v] = (ThreadParams){grille, hauteur, largeur, position, routeVerticale, 'V', &mutex};
                    placed = 1;
                }
            }
        }

        if (pthread_create(&threads[v], NULL, deplacerVoiture, &params[v]) != 0) {
            perror("Erreur lors de la création du thread");
            return EXIT_FAILURE;
        }
    }

    while(running) {
        afficherGrille(grille, hauteur, largeur);
        usleep(100000);
    }

    for (int v = 0; v < nombreVehicules; v++) {
        pthread_cancel(threads[v]);
    }

    for (int i = 0; i < hauteur; i++) {
        free(grille[i]);
    }
    free(grille);
    pthread_mutex_destroy(&mutex);

    printf("\nSimulation terminée\n");
    return EXIT_SUCCESS;
}

void afficherGrille(char **grille, int hauteur, int largeur) {
    pthread_mutex_lock(&mutex);
    system("clear");
    for (int i = 0; i < hauteur; i++) {
        for (int j = 0; j < largeur; j++) {
            printf("%c", grille[i][j]);
        }
        printf("\n");
    }
    printf("\nPress Ctrl+C to exit\n");
    pthread_mutex_unlock(&mutex);
}

void *deplacerVoiture(void *arg) {
    ThreadParams *params = (ThreadParams *)arg;
    
    while (running) {
        pthread_mutex_lock(params->mutex);
        
        if (params->direction == 'H') {
         
            params->grille[params->x][params->y] = '-';


            params->y += 1;


            if (params->y >= params->largeur) {
                pthread_mutex_unlock(params->mutex);
                return NULL;  
            }

           
            if (params->grille[params->x][params->y] == '-' || params->grille[params->x][params->y] == '|') {
                params->grille[params->x][params->y] = '*'; 
            } else {
                pthread_mutex_unlock(params->mutex);
                return NULL;  
            }
        } else if (params->direction == 'V') {
         
            params->grille[params->x][params->y] = '|';

        
            params->x += 1;

    
            if (params->x >= params->hauteur) {
                pthread_mutex_unlock(params->mutex);
                return NULL;  
            }

          
            if (params->grille[params->x][params->y] == '|') {
                params->grille[params->x][params->y] = '*';  
            } else {
                pthread_mutex_unlock(params->mutex);
                return NULL;  
            }
        }

        pthread_mutex_unlock(params->mutex);
        usleep(TEMPS_ATTENTE * 1000000);
    }
    
    return NULL;
}
