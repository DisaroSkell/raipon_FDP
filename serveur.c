// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"

int main(int argc, char *argv[]) {

  if(argc != 2){
    printf("lancement : ./serveur port\n");
    exit(1);
  }

  printf("Début programme\n");
  
  int * clients = (int*) malloc(100*sizeof(int));

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Erreur dans la création de la socket");
    exit(0);
  }
  printf("Socket Créé\n");

  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY;
  ad.sin_port = htons(atoi(argv[1]));
  int bd = bind(dS, (struct sockaddr*)&ad, sizeof(ad));
  if (bd == -1){
    perror("Erreur nommage de la socket");
    exit(0);
  }

  printf("Socket Nommé\n");

  int lstn = listen(dS, 7);
  if (lstn == -1){
    perror("Erreur passage en écoute");
    exit(0);
  }
  printf("Mode écoute\n");

  struct sockaddr_in aC;
  socklen_t lg = sizeof(struct sockaddr_in);

  pthread_t t[100];
  int i = 0; // Compteur de client


  while(i < 100){
    int dSC = accept(dS, (struct sockaddr*) &aC,&lg);

    struct traitement_params params;
    params.numclient = i;
    params.clienttab = clients;

    if (dSC == -1){
      perror("Erreur connexion non acceptée");
      exit(0);
    }
    printf("Client %d Connecté\n", i);
    pthread_t new;
    t[i] = new;
    clients[i]=dSC;
    printf("clients[%d] = %d\n", i, clients[i]);

    int thread = pthread_create(&t[i], NULL, traitement_serveur, &params);
    if (thread != 0){
      perror("Erreur création thread");
    }

    i++;
  }
  printf("Trop de client, arrêt du programme.");
}