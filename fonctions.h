struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
};

struct traitement_params {
  int numclient;
  int * clienttab;
};

void * reception(void * argpointer);
int envoi(int dS);
void * traitement_serveur(void * paramspointer);