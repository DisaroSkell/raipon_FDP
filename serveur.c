// Ce code a été créé à partir d'un template provenant de Mr GODEL

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

  printf("Début programme\n");

int dS = socket(PF_INET, SOCK_STREAM, 0);
if (dS == -1){
  perror("Erreur dans la création de la socket");
  exit(0);}
printf("Socket Créé\n");


struct sockaddr_in ad;
ad.sin_family = AF_INET;
ad.sin_addr.s_addr = INADDR_ANY ;
ad.sin_port = htons(atoi(argv[1])) ;
int bd = bind(dS, (struct sockaddr*)&ad, sizeof(ad));
if (bd == -1){
  perror("Erreur nommage de la socket");
  exit(0);}

printf("Socket Nommé\n");

int lstn = listen(dS, 7) ;
if (lstn == -1){
  perror("Erreur passage en écoute");
  exit(0);}
printf("Mode écoute\n");

struct sockaddr_in aC ;
socklen_t lg = sizeof(struct sockaddr_in) ;
int dSC = accept(dS, (struct sockaddr*) &aC,&lg);
if (dSC == -1){
  perror("Erreur connexion non acceptée");
  exit(0);}
  printf("Client Connecté\n");
while(1){
  size_t len = 0;
  ssize_t rcv_len = recv(dSC, &len, sizeof(len), 0) ;
  if (rcv_len == -1){perror("Erreur réception taille message");}
  printf("%d\n",(int)len);
  char * msg = (char *) malloc((len)*sizeof(char));
  ssize_t rcv = recv(dSC, msg, len, 0) ;
  if (rcv == -1){perror("Erreur réception message");}
  printf("Message reçu : %s\n", msg) ;
  
  int r = rcv ;

  int snd = send(dSC, &r, sizeof(int), 0) ;
  if (snd == -1){perror("Erreur envoi message");}
  printf("Message Envoyé\n");
  }
int sd1 = shutdown(dSC, 2) ; 
if (sd1 == -1){perror("Erreur shutdown");}
int sd2 = shutdown(dS, 2) ;
if (sd2 == -1){perror("Erreur shutdown");}
printf("Fin du programme");
}