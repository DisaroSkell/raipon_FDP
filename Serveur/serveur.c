// Ce code a été créé à partir d'un template provenant de M.Godel

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_serveur.h"
#include <semaphore.h>
#include <signal.h>

extern client clients[nb_clients_max*nb_channels_max];
extern sem_t sem_clients_max;
extern sem_t sem_tab_clients;

extern channel channels[nb_channels_max];
extern sem_t sem_channels_max;
extern sem_t sem_tab_channels;

int main(int argc, char *argv[]) {

  if(argc != 2){
    printf("lancement : ./serveur port\n");
    exit(1);
  }

  printf("Début programme\n");

  if (sem_init(&sem_clients_max, PTHREAD_PROCESS_SHARED, nb_clients_max*nb_channels_max) == -1) perror("Erreur dans la création de la sémaphore de création de threads");
  if (sem_init(&sem_tab_clients, PTHREAD_PROCESS_SHARED, 1) == -1) perror("Erreur dans la création de la sémaphore du tableau clients");

  if (sem_init(&sem_channels_max, PTHREAD_PROCESS_SHARED, nb_channels_max) == -1) perror("Erreur dans la création de la sémaphore de channels");
  if (sem_init(&sem_tab_channels, PTHREAD_PROCESS_SHARED, 1) == -1) perror("Erreur dans la création de la sémaphore du tableau de channels");
  
  // Client vide
  client init;
  init.socket = 0;
  init.pseudo = "";

  for (int i = 0; i < nb_clients_max*nb_channels_max; i++) {
    clients[i] = init;
  }

  // Channel vide
  channel blank;
  blank.nom = "";
  blank.description = "";

  for (int i = 0; i < nb_clients_max; i++) {
    blank.occupants[i] = init;
  }

  for (int i = 0; i < nb_channels_max; i++) {
    channels[i] = blank;
  }

  // Channel d'essai, à supprimer
  channel essai;
  essai.nom = (char *) malloc(6*sizeof(char));
  strcpy(essai.nom, "Fleur");
  essai.description = (char *) malloc(25*sizeof(char));
  strcpy(essai.description, "Le channel des FLEUUUURS");

  for (int i = 0; i < nb_clients_max; i++) {
    essai.occupants[i] = init;
  }

  channels[0] = essai;
  // Fin de la section à supprimer

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

  pthread_t t[nb_clients_max*nb_channels_max];
  int i; // Numéro de client courant

  // Écoute du ctrl c
  signal(SIGINT, signal_handle);

  while(1){
    if (sem_wait(&sem_clients_max) == -1) perror("Erreur blocage sémaphore");

    int dSC = accept(dS, (struct sockaddr*) &aC,&lg);

    if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
    i = chercher_place(nb_clients_max*nb_channels_max, clients);
    sem_post(&sem_tab_clients);

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

      sem_post(&sem_clients_max);

      continue; // On va à la prochaine boucle
    }
    else {
      if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
      if (chercher_client(pseudo, nb_clients_max*nb_channels_max, clients) != -1) {
        perror("Pseudo déjà utilisé !");

        envoi_message(dSC, "Pseudo déjà utilisé.\n");
        
        sem_post(&sem_clients_max);
        sem_post(&sem_tab_clients);

        continue; // On va à la prochaine boucle
      }
      else if (strcmp(pseudo, "Serveur") == 0) {
        perror("Quelqu'un veut s'appeler Serveur !");

        envoi_message(dSC, "Vous ne pouvez pas vous appeler Serveur !\n");

        sem_post(&sem_clients_max);
        sem_post(&sem_tab_clients);

        continue; // On va à la prochaine boucle
      }
      else printf("%s s'est connecté !\n", pseudo);
      sem_post(&sem_tab_clients);
    }

    envoi_message(dSC, "Bienvenue sur le serveur !\n");

    pthread_t new;
    t[i] = new;

    if (sem_wait(&sem_tab_clients) == -1) perror("Erreur blocage sémaphore");
    clients[i].socket = dSC;
    clients[i].pseudo = pseudo;
    printf("clients[%d] = %d\n", i, clients[i].socket);
    sem_post(&sem_tab_clients);

    int thread = pthread_create(&t[i], NULL, traitement_serveur, &params);
    if (thread != 0){
      perror("Erreur création thread");
    }
  }
  sem_destroy(&sem_clients_max);
  sem_destroy(&sem_tab_clients);

  sem_destroy(&sem_channels_max);
}