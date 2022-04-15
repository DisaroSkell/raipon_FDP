// Ce code a été créé à partir d'un template provenant de Mr GODEL


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "fonctions.h"

struct argsrec {
  int socket;
};

void * reception2(void * argpointer){
  struct argsrec * args = argpointer;
  ssize_t len = 0;
  ssize_t rcv_len = recv(args->socket, &len, sizeof(len), 0) ;
  if (rcv_len == -1){perror("Erreur réception taille message");}
  printf("%d\n",(int)len);
  char * msg = (char *) malloc((len)*sizeof(char));
  ssize_t rcv = recv(args->socket, msg, len, 0) ;
  if (rcv == -1){perror("Erreur réception message");}
  printf("Message reçu : %s\n", msg);
}

int main(int argc, char *argv[]) {

  if(argc != 3){
    printf("lancement : ./client IPServeur port\n");
    exit(1);
  }

  printf("Début programme\n");

  int dS = socket(PF_INET, SOCK_STREAM, 0);
  if (dS == -1){
    perror("Erreur dans la création de la socket");
    exit(0);
  }
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in);

  int co = connect(dS, (struct sockaddr *) &aS, lgA) ;
  if (co == -1){
    perror("Erreur envoi demmande de connexion");
    exit(0);}
  printf("Socket Connecté\n");

  pthread_t idt;

  struct argsrec argsf;

  argsf.socket = dS;
  
  int thread = pthread_create(&idt,NULL,reception2,&argsf);

  if (thread == 0) {
    perror("Erreur de création de thread");
    exit(0);
  }

  while(1){
    envoi(dS);
  }

  int sd = shutdown(dS,2) ;
  if (sd == -1){perror("Erreur shutdown");}
  printf("Fin du programme");
}