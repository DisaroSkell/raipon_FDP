// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"
#include <semaphore.h>
#include <signal.h>

extern client clients[nb_client_max];
extern sem_t semaphore;
extern sem_t semaphoreCli;
extern int serveur = 0;

int main(int argc, char *argv[]) {

  if(argc != 2){
    printf("lancement : ./serveur port\n");
    exit(1);
  }

  printf("Début programme\n");

  if (sem_init(&semaphore, PTHREAD_PROCESS_SHARED, nb_client_max) == -1) perror("Erreur dans la création de la sémaphore de création de threads");
  if (sem_init(&semaphoreCli, PTHREAD_PROCESS_SHARED, 1) == -1) perror("Erreur dans la création de la sémaphore du tableau clients");
  
  client init;
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
  serveur = dS;
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
  int i; // Numéro de client courant

  //écoute ctrl c
  signal(SIGINT, signal_handle);

  while(1){
    if (sem_wait(&semaphore) == -1) perror("Erreur blocage sémaphore");

    int dSC = accept(dS, (struct sockaddr*) &aC,&lg);

    if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
    i = chercher_place();
    sem_post(&semaphoreCli);

    if (i == -1) {
      perror("Aucune place n'a été trouvée, c'est pas normal.");
      exit(0);
    }

    traitement_params params;
    params.numclient = i;

    if (dSC == -1){
      perror("Erreur connexion non acceptée");
      exit(0);
    }

    char * pseudo = (char*) malloc(taille_pseudo*sizeof(char));
    ssize_t rcv_pseudo = recv(dSC, pseudo, taille_pseudo, 0);
    if (rcv_pseudo == -1) {
      perror("Erreur réception pseudo");

      envoi_message(dSC, "Pseudo mal reçu.\n");

      sem_post(&semaphore);

      continue; // On va à la prochaine boucle
    }
    else {
      if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
      if (chercher_client(pseudo) != -1) {
        perror("Pseudo déjà utilisé !");

        envoi_message(dSC, "Pseudo déjà utilisé.\n");
        
        sem_post(&semaphore);
        sem_post(&semaphoreCli);

        continue; // On va à la prochaine boucle
      }
      else printf("%s s'est connecté !\n", pseudo);
      sem_post(&semaphoreCli);
    }

    envoi_message(dSC, "Bienvenue sur le serveur !\n");

    pthread_t new;
    t[i] = new;

    if (sem_wait(&semaphoreCli) == -1) perror("Erreur blocage sémaphore");
    clients[i].socket = dSC;
    clients[i].pseudo = pseudo;
    printf("clients[%d] = %d\n", i, clients[i].socket);
    sem_post(&semaphoreCli);

    int thread = pthread_create(&t[i], NULL, traitement_serveur, &params);
    if (thread != 0){
      perror("Erreur création thread");
    }
  }
  sem_destroy(&semaphore);
  sem_destroy(&semaphoreCli);
}