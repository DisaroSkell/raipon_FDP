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

    int numclient = params->numclient;

    size_t len = 0;
    printf("%d: Client socket = %d\n", numclient, params->clienttab[numclient]);
    while (1) {
        ssize_t rcv_len = recv(params->clienttab[numclient], &len, sizeof(len), 0);
        if (rcv_len == -1) perror("Erreur réception taille message");
        printf("%d: Longueur du message reçu: %d\n", numclient, (int)len);

        char * msg = (char *) malloc((len)*sizeof(char));
        ssize_t rcv = recv(params->clienttab[numclient], msg, len, 0);
        if (rcv == -1) perror("Erreur réception message");
        else {
            printf("%d: Message reçu : %s\n", numclient, msg);
        }

        int end = strcmp(msg, "fin\n");

        if (end == 0) {
            char * messagedeco = "Déconnexion en cours...\n";
            int tailledeco = strlen(messagedeco);
            int senddecotaille = send(params->clienttab[numclient], &tailledeco, sizeof(tailledeco), 0);
            int senddeco = send(params->clienttab[numclient], messagedeco, tailledeco, 0);
            printf("%d: Fin du thread\n", numclient);
            pthread_exit(0);
        }

        for(int i=0; i<=100; i++){
            if (params->clienttab[i] != params->clienttab[numclient] && params->clienttab[i] != 0) {
                printf("%d: client = %d\n", numclient, params->clienttab[i]);
                int snd2 = send(params->clienttab[i], &len, sizeof(len), 0);
                if (snd2 == -1) perror("Erreur envoi taille message");
                int snd = send(params->clienttab[i], msg, len , 0);
                if (snd == -1) perror("Erreur envoi message");
                printf("%d: Message Envoyé au client %d: %s\n", numclient, params->clienttab[i], msg);
            }
        }
        free(msg);
    }
}