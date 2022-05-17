#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_client.h"
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

int socketServeur;

void * thread_reception(void * argpointer){
    argsrec * args = argpointer;
    int len = 0;
    while(!(args->fin)) {
        ssize_t rcv_len = recv(args->socket, &len, sizeof(len), 0);
        if (rcv_len == -1) {
            perror("Erreur réception taille message");
            exit(0);
        }
        if (rcv_len == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(args->socket, msg, len, 0);
        if (rcv == -1) {
            perror("Erreur réception message");
            exit(0);
        }
        if (rcv == 0) {
            printf("Non connecté au serveur, fin du thread\n");
            exit(0);
        }
        else {
            if (memcmp(msg, "/ef ", strlen("/ef ")) == 0) {
                char * msgmodif = (char *) malloc((strlen(msg)-4)*sizeof(char));
                strcpy(msgmodif, strtok(msg, "/")); // On enlève le début du message
                
                char * token = strtok(msgmodif, " "); // On s'en fiche un peu du début
                token = strtok(NULL, " "); // On regarde le nom du fichier

                if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                    perror("Il manque un nom de fichier après /ef !");
                    continue;;
                }
                
                char * nomf = (char *) malloc(strlen(token)*sizeof(char));
                strcpy(nomf, token);

                token = strtok(NULL, " "); // On regarde la suite, ici : la taille du fichier

                if (token == NULL) { // Ne devrait pas arriver, mais on sait jamais
                    perror("Il manque la taille du fichier après son nom !");
                    continue;
                }

                int taillef = atoi(token);

                recup_fichier(args->socket, nomf, taillef);
            } else {
                printf("\n");
                printf("%s", msg);
            }
        }
    }
}

int lecture_message(int dS) {
    printf("Entrez un message\n");

    // On lit le message dans le terminal
    int taille = 30;
    char * m = (char *) malloc(taille*sizeof(char));
    fgets(m, taille*sizeof(char), stdin);

    if (strcmp(m, "/envoifichier\n") == 0 || strcmp(m, "/ef\n") == 0) {
        // On montre au client les fichiers disponibles à l'envoi
        print_repertoire();
    } else if (strncmp(m, "/envoificher ", 13) == 0 || strncmp(m, "/ef ", 4) == 0) {
        // On envoie un ficher
        char * token = strtok(m, " ");
        token = strtok(NULL, " "); // On regarde le nom de fichier qui est censé se situer juste après le nom de commande

        if (token == NULL) {
            // On est gentil, on fait comme s'il demandait la liste des fichiers :)
            print_repertoire();
        }
        else {
            token = strtok(token, "\n");

            envoi_fichier(dS, token);
        }
    }
    else envoi_message(dS, m);

    return strcmp(m, "/fin\n") != 0;
}

int envoi_message(int socket, char * msg) {
    int resultat = 0;
    
    int len= strlen(msg)+1;

    char * message = (char *) malloc(len*sizeof(char));
    strcpy(message, msg);

    int sndlen = send(socket, &len, sizeof(len), 0);
    if (sndlen == -1) {
        perror("Erreur envoi taille message");
        resultat = -1;
    }
    else {
        int sndmsg = send(socket, message, len, 0);
        if (sndmsg == -1) {
            perror("Erreur envoi message");
            resultat = -1;
        }
        else printf("Message envoyé:\n%s\n", message);
    }

    return resultat;
}

void print_repertoire() {
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;
    mydir = opendir("Public");
    while ((myfile = readdir(mydir)) != NULL) {
        stat(myfile->d_name, &mystat);
        printf("%ld",mystat.st_size);
        printf(" %s\n", myfile->d_name);
    }
}

void envoi_fichier(int socket, char * nomfichier) {
    FILE *fp;

    // On met le chemin relatif du fichier dans un string
    char * nomchemin = (char *) malloc((strlen(nomfichier)+7)*sizeof(char));
    strcpy(nomchemin, "Public/");
    strcat(nomchemin, nomfichier);

    fp = fopen(nomchemin, "r");
    if (fp == NULL) {
        perror("Erreur durant la lecture du fichier");
        return;
    }

    char data[SIZE] = {0};

    long int taillefichier;

    if (fp) {
        fseek (fp, 0, SEEK_END);
        taillefichier = ftell (fp);
        fseek (fp, 0, SEEK_SET);
    } else {
        perror("Problème dans la lecture du fichier");
    }

    // La commande sera "/ef [nom fichier] [taille fichier]\0"
    // D'où les additions:
    char * commande = (char *) malloc((3 + strlen(nomfichier) + 1 + tailleint(taillefichier) + 1) * sizeof(char));

    strcpy(commande, "/ef ");
    strcat(commande, nomfichier);
    strcat(commande, " ");

    char * taillef = (char *) malloc((tailleint(taillefichier) + 1) * sizeof(char));
    sprintf(taillef, "%ld", taillefichier);
    strcat(commande, taillef);
    
    envoi_message(socket, commande);

    char * mess = (char *) malloc(50*sizeof(char));
    int rep = recv(socket, mess, 50*sizeof(char), 0); //Feedback du serveur "ok" ou mess d'erreur
    if (rep == -1){
            perror("Erreur dans la réception du feedback");
        }

    if (strcmp(mess,"ok") != 0) { 
        perror(mess);
        return;
    }

    // Après avoir envoyé la commande, on envoie le fichier
    while(fgets(data, SIZE, fp) != NULL) {
        ssize_t envoi = send(socket, data, SIZE, 0);
        if (envoi == -1) {
            perror("Erreur dans l'envoi du fichier");
            return;
        }

        bzero(data, SIZE);
    }

    printf("Fichier envoyé !\n");
}

void recup_fichier(int socket, char * nomfichier, long taillefichier) {
    FILE *fp;
    char buffer[SIZE];

    // On met le chemin relatif du fichier dans un string
    char * cheminf = (char *) malloc((strlen(nomfichier)+7)*sizeof(char));
    strcpy(cheminf, "Public/");
    strcat(cheminf, nomfichier);
    
    fp = fopen(cheminf, "w");
    
    ssize_t rcv_f;
    while (ftell(fp) < taillefichier) { // On s'arrête quand on a reçu l'équivalent de la taille du fichier
        rcv_f = recv(socket, buffer, SIZE, 0);
        if (rcv_f == -1) {
            perror("Erreur de reception du fichier !");
            return;
        }
        if (rcv_f == 0) {
            perror("Erreur de connexion au client !");
            return;
        }

        fprintf(fp, "%s", buffer);
    }

    fclose(fp);
    printf("Fin de la réception du fichier %s\n", nomfichier);
}

void signal_handleCli(int sig){
    envoi_message(socketServeur, "/fin\n");

    printf("Fin du programme\n");
    exit(0);
}

int tailleint(int nb) {
    int nombre = nb;
    int resultat = 1;

    // On compte les chiffres en divisant par 10 à chaque fois
    while (nombre >= 10) {
        nombre = nombre/10;
        resultat++;
    }

    return resultat;
}