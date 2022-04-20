struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
};

struct traitement_params {
  int numclient;
  int * clienttab;
};

struct commande {
  // L'id de la commande: -1 = problème dans la fonction; 0 = pas de commande; 1 = /;
  int id_op;
  char * nom_cmd; // Le nom de la commande qui suit l'opérateur
  char * message;
  int user;
};

void * reception(void * argpointer);
int envoi(int dS);
void * traitement_serveur(void * paramspointer);
struct commande gestion_commande(char * msg);