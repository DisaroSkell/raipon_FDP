// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"

int * clients = (int*) malloc(100*sizeof(int));

void* traitement_serveur(int dSC){
    size_t len = 0;
    printf("hhhhhhhhhhhhhh = %d", dSC);
    ssize_t rcv_len = recv(dSC, &len, sizeof(len), 0) ;
    if (rcv_len == -1){perror("Erreur réception taille message");}
    printf("%d\n",(int)len);
    char * msg = (char *) malloc((len)*sizeof(char));
    ssize_t rcv = recv(dSC, msg, len, 0) ;
    if (rcv == -1){perror("Erreur réception message");}
    printf("Message reçu : %s\n", msg) ;

    int end = strcmp(msg, "fin\n");

    if (end == 0) {
        //pthread_exit(p.th);
        printf("Fin du thread");
    }
    for(int i; i<=100; i++){
        if (clients[i] != dSC) {
          printf("clients = %d", clients[i]);
            int snd2 = send(clients[i], &len, sizeof(len), 0) ;
            if (snd2 == -1){perror("Erreur envoi taille message");}
            int snd = send(clients[i], msg, len , 0) ;
            if (snd == -1){perror("Erreur envoi message");}
            printf("Message Envoyé\n");
        }
    }
}


int main(int argc, char *argv[]) {

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Erreur dans la création de la socket");
    exit(0);
  }
  printf("Socket Créé\n");


  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ;
  int bd = bind(dS, (struct sockaddr*)&ad, sizeof(ad));
  if (bd == -1){
    perror("Erreur nommage de la socket");
    exit(0);
  }

  printf("Socket Nommé\n");

  int lstn = listen(dS, 7) ;
  if (lstn == -1){
    perror("Erreur passage en écoute");
    exit(0);
  }
  printf("Mode écoute\n");

  struct sockaddr_in aC ;
  socklen_t lg = sizeof(struct sockaddr_in) ;

  pthread_t t[100];
  int i = 0;
  int c = 0;


  while(1){
    int dSC = accept(dS, (struct sockaddr*) &aC,&lg);
    if (dSC == -1){
    perror("Erreur connexion non acceptée");
    exit(0);
    }
    printf("Client Connecté\n");
    pthread_t new;
    t[i] = new;
    clients[c]=dSC;
    printf("clients[0] = %d", clients[0]);

    int thread = pthread_create(&t[i], NULL, traitement_serveur, dSC);
    if (thread != 0){
      perror("Erreur création thread");
    }

  }
}