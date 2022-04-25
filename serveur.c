// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"
#include <semaphore.h>

extern client clients[nb_client_max];
sem_t semaphore;

int main(int argc, char *argv[]) {

  if(argc != 2){
    printf("lancement : ./serveur port\n");
    exit(1);
  }

  printf("Début programme\n");

  client init;
  int init_semaphore sem_init(&semaphore, PTHREAD_PROCESS_SHARED, 3);
  if (init_semaphore == -1) {
    perror("Erreur dans la création de la sémaphore");
  }
  init.socket = 0;
  init.pseudo = "";

  for (int i = 0; i < nb_client_max; i++) {
    clients[i] = init;
  }

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

  pthread_t t[nb_client_max];
  int i = 0; // Compteur de client


  while(i < nb_client_max){
    int dSC = accept(dS, (struct sockaddr*) &aC,&lg);

    traitement_params params;
    params.numclient = i;

    if (dSC == -1){
      perror("Erreur connexion non acceptée");
      exit(0);
    }
    size_t len = 30;
    char * msg = (char *) malloc((len)*sizeof(char));
    strcpy(msg, "Bienvenue sur le serveur !\n");
    int sndmsg = send(dSC, msg, len, 0);
    if (sndmsg == -1) perror("Erreur envoi message bienvenue");
        
    printf("Message de bienvenue envoyé au client %d\n", dSC);

    char * pseudo = (char*) malloc(taille_pseudo*sizeof(char));
    ssize_t rcv_pseudo = recv(dSC, pseudo, taille_pseudo, 0);
    if (rcv_pseudo == -1) perror("Erreur réception taille message");
    else printf("%s s'est connecté !\n", pseudo);

    pthread_t new;
    t[i] = new;
    clients[i].socket = dSC;
    clients[i].pseudo = pseudo;
    printf("clients[%d] = %d\n", i, clients[i].socket);

    int thread = pthread_create(&t[i], NULL, traitement_serveur, &params);
    if (thread != 0){
      perror("Erreur création thread");
    }

    i++;
  }
  printf("Trop de client, arrêt du programme.");
}