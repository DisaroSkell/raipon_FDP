#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"

void * reception(void * argpointer){
    struct argsrec * args = argpointer;
    while(!(args->fin)) {
        ssize_t len = 0;
        ssize_t rcv_len = recv(args->socket, &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(args->socket, msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        printf("Message reçu : %s\n", msg);
    }
}

int envoi(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(30*sizeof(char));
    fgets( m, 30*sizeof(char), stdin );
    size_t len= strlen(m)+1;
    int snd2 = send(dS, &len, sizeof(len), 0);
    if (snd2 == -1) perror("Erreur envoi taille message");
    int snd = send(dS, m, len , 0);
    if (snd == -1) perror("Erreur envoi message");
    printf("Message Envoyé \n");
    return strcmp(m, "fin\n") != 0;
}

void* traitement_serveur(void * paramspointer){

    struct traitement_params * params = paramspointer;

    size_t len = 0;
    printf("Client socket = %d\n", params->clienttab[params->numclient]);
    while (1) {
        ssize_t rcv_len = recv(params->clienttab[params->numclient], &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        printf("Longueur du message reçu: %d\n",(int)len);
        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(params->clienttab[params->numclient], msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        else {
            printf("Message reçu : %s\n", msg);
        }

        int end = strcmp(msg, "fin\n");

        if (end == 0) {
            char * messagedeco = "Déconnexion en cours...\n";
            int tailledeco = strlen(messagedeco);
            int senddecotaille = send(params->clienttab[params->numclient], &tailledeco, sizeof(tailledeco), 0);
            int senddeco = send(params->clienttab[params->numclient], messagedeco, tailledeco, 0);
            printf("Fin du thread\n");
            pthread_exit(0);
        }

        printf("Pour rappel, je suis le thread en charge du client %d\n", params->numclient);
        for(int i; i<=100; i++){
            if (params->clienttab[i] != params->clienttab[params->numclient] && params->clienttab[i] != 0) {
                printf("client = %d\n", params->clienttab[i]);
                int snd2 = send(params->clienttab[i], &len, sizeof(len), 0);
                if (snd2 == -1) perror("Erreur envoi taille message");
                int snd = send(params->clienttab[i], msg, len , 0);
                if (snd == -1) perror("Erreur envoi message");
                printf("Message Envoyé\n");
            }
        }
    }
}