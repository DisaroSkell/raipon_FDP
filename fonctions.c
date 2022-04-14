#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "fonctions.h"

void envoi(int dS) {
    printf("Entrez un message\n");
    char * m = (char *) malloc(30*sizeof(char));
    fgets( m, 30*sizeof(char), stdin ); 
    size_t len= strlen(m)+1;
    int snd2 = send(dS, &len, sizeof(len),0);
    if (snd2 == -1){perror("Erreur envoi taille message");}
    int snd = send(dS, m, len , 0) ;
    if (snd == -1){perror("Erreur envoi message");}
    printf("Message Envoyé \n");
}

void reception(int dS){
    ssize_t len = 0;
    ssize_t rcv_len = recv(dS, &len, sizeof(len), 0) ;
    if (rcv_len == -1){perror("Erreur réception taille message");}
    printf("%d\n",(int)len);
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(dS, msg, len, 0) ;
    if (rcv == -1){perror("Erreur réception message");}
    printf("Message reçu : %s\n", msg);
}

void* traitement_serveur(struct params p){
    size_t len = 0;
    printf("%d", p.origine);
    ssize_t rcv_len = recv(p.*origine, &len, sizeof(len), 0) ;
    if (rcv_len == -1){perror("Erreur réception taille message");}
    printf("%d\n",(int)len);
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(p.origine, msg, len, 0) ;
    if (rcv == -1){perror("Erreur réception message");}
    printf("Message reçu : %s\n", msg) ;

    int end = strcmp(msg, "fin\n");

    if (end == 0) {
        pthread_exit(p.th);
        printf("Fin du thread");
    }
    for(int i; i<=100; i++){
        if (p.destinataires[i] != p.origine) {
            int snd2 = send((long)p.destinataires[i], &len, sizeof(len), 0) ;
            if (snd2 == -1){perror("Erreur envoi taille message");}
            int snd = send((long)p.destinataires[i], msg, len , 0) ;
            if (snd == -1){perror("Erreur envoi message");}
            printf("Message Envoyé\n");
        }
    }
}