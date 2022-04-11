// Ce code a été créé à partir d'un template provenant de Mr GODEL


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "fonctions.h"
//Ajouter le fonctions.c dans la compilation


int main(int argc, char *argv[]) {

    if(argc != 4){
    printf("lancement : ./client IPServeur port numéroClient\n");
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
socklen_t lgA = sizeof(struct sockaddr_in) ;
int num = atoi(argv[3]);
if (numm != 1 || num != 2) {
  perror("Argument numéro client invalide");
  exit(0);
}
int co = connect(dS, (struct sockaddr *) &aS, lgA) ;
if (co == -1){
  perror("Erreur envoi demmande de connexion");
  exit(0);}
printf("Socket Connecté\n");


while(1){
  if (num == 1) {
    envoi(dS);
    reception(dS);
  }
  else {
    reception(dS);
    envoi(dS);
  }
  

int sd = shutdown(dS,2) ;
if (sd == -1){perror("Erreur shutdown");}
printf("Fin du programme");
}