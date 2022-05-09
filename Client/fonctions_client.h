#define taille_pseudo 30 // taille du pseudo en comptant le \0

typedef struct argsrec {
  int socket;
  int fin; // Pointer sur booléen, vrai ssi le programme doit se terminer
}argsrec;

// Le thread de reception de messages du client
void * thread_reception(void * argpointer);

// Lecture des messages dans le terminal
int lecture_message(int dS);

// Envoi d'un message par le socket (envoi de la taille puis du message)
int envoi_message(int socket, char * msg);

// Fonction de signal CTRL C du client
void signal_handleCli(int sig);