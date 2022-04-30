#define nb_client_max 3
#define taille_pseudo 30 // taille du pseudo en comptant le \0

typedef struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
}argsrec;

typedef struct client {
  int socket;
  char * pseudo;
}client;

typedef struct traitement_params {
  int numclient;
}traitement_params;

typedef struct commande {
  // L'id de la commande: -1 = problème dans la fonction; 0 = pas de commande; 1 = /;
  int id_op;
  char * nom_cmd; // Le nom de la commande qui suit l'opérateur
  char * message;
  char * user;
}commande;

void * reception(void * argpointer);
int lecture_message(int dS);
void envoi_message(char * msg, int socket);
void * traitement_serveur(void * paramspointer);
void envoi_serveur(int numclient, int numreceveur, char * msg);
int chercher_client(char * pseudo);
int chercher_place();
char * lire_manuel();
commande gestion_commande(char * msg);