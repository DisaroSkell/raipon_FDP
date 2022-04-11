// Ce code a été créé à partir d'un template provenant de Mr GODEL


#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

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

void reception(int dS)
  int r;
  ssize_t rcv = recv(dS, &r, sizeof(int), 0) ;
  if (rcv == -1){perror("Erreur réception message");};
  printf("Réponse reçue : %d\n", r) ;
  }
}

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