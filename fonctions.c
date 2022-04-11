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

void traitement_serveur(int dS, int origine, int destinataire){
  size_t len = 0;
  ssize_t rcv_len = recv(origine, &len, sizeof(len), 0) ;
  if (rcv_len == -1){perror("Erreur réception taille message");}
  printf("%d\n",(int)len);
  char * msg = (char *) malloc((len)*sizeof(char));
  ssize_t rcv = recv(origine, msg, len, 0) ;
  if (rcv == -1){perror("Erreur réception message");}
  printf("Message reçu : %s\n", msg) ;

  int end = strcmp(msg, "fin\n");

  if (end == 0) {
    int sd1 = shutdown(origine, 2) ; 
    int sd2 = shutdown(dS, 2) ;
    int sd3 = shutdown(destinataire, 2) ;
    if (sd2 == -1 || sd2 == -1 || sd3 == -1){perror("Erreur shutdown");}
    printf("Fin du programme");
  }

  int snd2 = send(destinataire, &len, sizeof(len), 0) ;
  if (snd2 == -1){perror("Erreur envoi taille message");}
  int snd = send(destinataire, msg, len , 0) ;
    if (snd == -1){perror("Erreur envoi message");}
  printf("Message Envoyé\n");
  }