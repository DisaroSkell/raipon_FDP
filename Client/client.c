// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions_client.h"
#include <signal.h>

extern int socketServeur;
extern char * IPServeur;
extern int portServeur;

int main(int argc, char *argv[]) {

  if(argc != 4){
    printf("lancement : ./client Pseudo IPServeur port\n");
    exit(1);
  }

  if(strlen(argv[1]) >= taille_pseudo) {
    printf("Pseudo trop long, entrez un pseudo de moins de %d caractères.\n", taille_pseudo);
    exit(1);
  }

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Erreur dans la création de la socket");
    exit(0);
  }
  printf("Socket Créé\n");

  socketServeur = dS;

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[2],&(aS.sin_addr));
  aS.sin_port = htons(atoi(argv[3]));
  char * IPServeur = argv[2];
  int portServeur = atoi(argv[3]);
  socklen_t lgA = sizeof(struct sockaddr_in);

  int co = connect(dS, (struct sockaddr *) &aS, lgA);
  if (co == -1){
    perror("Erreur envoi demmande de connexion");
    exit(0);
  }
  printf("Socket Connecté\n");

  printf("Envoi du pseudo au serveur...\n");

  int snd = send(dS, argv[1], taille_pseudo, 0);
  if (snd == -1) perror("Erreur envoi pseudo au serveur");
  else printf("Pseudo envoyé au serveur\n");

  printf("Demande de connexion au serveur...\n");


  int len;
  ssize_t rcv_len = recv(dS, &len, sizeof(len), 0);
  if (rcv_len == -1) {
    perror("Erreur réception taille message");
    exit(0);
  }
  if (rcv_len == 0) {
    printf("Non connecté au serveur, fin du thread\n");
    exit(0);
  }

  char * msg = (char *) malloc((len)*sizeof(char));
  ssize_t rcv = recv(dS, msg, len, 0);
  if (rcv == -1) {
    perror("Erreur réception message de bienvenue");
    exit(0);
  }
  else if (rcv == 0) {
    printf("Non connecté au serveur, fin du thread\n");
    exit(0);
  }

  // On arrête tout si c'est pas le message de bienvenu.
  if (strcmp(msg, "Bienvenue sur le serveur !\n") != 0) {
    printf("Mauvais message de bienvenue reçu: %s\nOn arrête tout.\n", msg);
    exit(0);
  }
  else printf("%s\n", msg);
  free(msg);

  pthread_t idt;

  argsrec argsf;

  argsf.socket = dS;
  argsf.fin = 0;

  int thread = pthread_create(&idt,NULL,thread_reception,&argsf);

  if (thread != 0) {
    perror("Erreur de création de thread");
    exit(0);
  }

  // Écoute du ctrl c
  signal(SIGINT, signal_handleCli);

  while(lecture_message(dS)){}

  argsf.fin = 1;
  pthread_join(idt,NULL);

  int sd = shutdown(dS,2);
  if (sd == -1) perror("Erreur shutdown");
  printf("Fin du programme\n");
}