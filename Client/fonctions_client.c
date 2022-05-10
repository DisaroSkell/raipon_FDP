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
#define SIZE 1024

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
            printf("Message reçu :\n");
            printf("%s\n", msg);
        }
    }
}

int lecture_message(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(50*sizeof(char));
    fgets(m, 30*sizeof(char), stdin);
    char * token = strtok(m, " "); // On segmente la commande avec les espaces

    if (strcmp(m,"/fichier\n") == 0) {
        envoi_repertoire(dS);
    }
    else if (strcmp(token,"/file\n") == 0) {
        perror("Vous devez mettre le nom du fichier après /file !");
    }
    else if (strcmp(token,"/file") == 0) {
        char * nomfichier = (char *) malloc(20*sizeof(char));
        char * taillef = (char *) malloc(5*sizeof(char));
        token = strtok(NULL, " ");
        if (token == NULL) {
                perror("Vous devez mettre le nom du fichier après /file !");
            }
        strcpy(token,nomfichier);
        token = strtok(NULL, " ");
        if (token == NULL) {
                perror("Vous devez mettre la taille du fichier après son nom !");
            }
        strcpy(token,taillefichier);
        int taillefichier = atoi(taillef);
        envoi_fichier(dS, nomfichier, taillefichier);
    }
    else envoi_message(dS, m);

    return strcmp(m, "/fin\n") != 0;
}

// Envoie par le socket
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

void envoi_repertoire(int socket) {
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

void envoi_fichier(int socket, char * nomfichier, int taillefichier) {
    FILE *fp;
    fp = fopen(nomfichier, "r");
    if (fp == NULL) {
        perror("Erreur durant la lecture du fichier");
        exit(1);
    }
    int n;
    char data[SIZE] = {0};
    
    char * message = (char *) malloc(10*sizeof(char));
    strcpy(message, "/file");
    envoi_message(socket,message);
    while(fgets(data, SIZE, fp) != NULL) {
        if (send(socket, data, sizeof(data), 0) == -1) {
        perror("[-]Error in sending file.");
        exit(1);
    }
    bzero(data, SIZE);
  }
}

void signal_handleCli(int sig){
    envoi_message(socketServeur, "Cette personne se déconnecte.\n");

    printf("Fin du programme\n");
    exit(0);
}