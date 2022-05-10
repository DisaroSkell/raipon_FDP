#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_client.h"
#include <semaphore.h>
#include <signal.h>

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

    if (strcmp(m,"/fichier"))
    envoi_message(dS, m);

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
    char * msg;
    mydir = opendir("Public");
    while ((myfile = readdir(mydir)) != NULL) {
        msg = (char *) malloc(30*sizeof(char));
        stat(myfile->d_name, &mystat);
        sprintf(msg,"%ld %s",mystat.st_size,myfile->d_name);
        printf("%s\n",msg);
        envoi_message(socket,msg);
        free(msg);
    }
}

void signal_handleCli(int sig){
    envoi_message(socketServeur, "Cette personne se déconnecte.\n");

    printf("Fin du programme\n");
    exit(0);
}