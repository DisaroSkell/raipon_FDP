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

// Les envoie renvoient 0 si tout se passe bien et -1 s'il y a une erreur.
void * reception(void * argpointer);
int lecture_message(int dS);
int envoi_message(int socket, char * msg);
void * traitement_serveur(void * paramspointer);
int envoi_direct(int numreceveur, char * msg);
int chercher_client(char * pseudo);
int chercher_place();
char * lire_manuel();
commande gestion_commande(char * msg);
void signal_handle(int sig);
void signal_handleCli(int sig);